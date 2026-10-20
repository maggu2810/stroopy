// SPDX-License-Identifier: GPL-3.0-or-later
// A multiplayer room over Nostr (docs/spec/protocol/v1.md §4): presence, host, starting games
// and everybody's progress. The game itself runs locally; this only reports.
import { computed, signal, untracked, type Signal } from "@preact/signals";
import { RULES_VERSION } from "../core/rules-v1.ts";
import { encodeText } from "../core/share-code-v1.ts";
import { createEvent, encodeMessage, KIND, newSecretKey, parseMessage, publicKey, roomCode, roomTag, type GameState, type Message, type NostrEvent } from "./protocol-v1.ts";
import { RelayPool } from "./relay-pool.ts";

export const HEARTBEAT_MS = 5000;
export const MIN_STATUS_INTERVAL_MS = 500;
export const PRESENCE_TIMEOUT_MS = 15000;
const SUPPORTED_RULES = [RULES_VERSION];

export interface Player {
  key: string;
  name: string;
  host: boolean;
  rules: number[];
  mode?: number;
  game?: GameState;
  lastSeen: number;
  me: boolean;
}

/** What the room needs from the local game. */
export interface LocalGame {
  /** Start the game of `code` (rules, mode, seed decoded). */
  play(code: string, mode: number, seed: bigint): void;
  /** True while a countdown or game runs. */
  busy(): boolean;
}

export class Room {
  readonly id: string;
  readonly code: string;
  readonly players = signal<Player[]>([]); // others; see `everyone`
  readonly me: Signal<Player>;
  readonly connected: RelayPool["connected"];
  readonly relayCount: number;
  /** Host's selected mode for the next game. */
  readonly mode = signal(1);
  /** A newer game the host started while we were still playing. */
  readonly pending = signal<{ code: string; mode: number; seed: bigint } | null>(null);
  readonly everyone = computed(() => [this.me.value, ...this.players.value]);
  /** Players who left after playing; their results stay in the ranking. */
  readonly departed = signal<Player[]>([]);
  readonly hostKey = computed(() => {
    const claimants = this.everyone.value.filter((p) => p.host).map((p) => p.key);
    return claimants.sort()[0] ?? null;
  });
  readonly isHost = computed(() => this.hostKey.value === this.me.value.key);
  /** Rules versions every present player can play. */
  readonly commonRules = computed(() => SUPPORTED_RULES.filter((r) => this.players.value.every((p) => p.rules.includes(r))));

  private readonly secret = newSecretKey();
  private readonly pool: RelayPool;
  private played = new Set<string>();
  private lastSent = 0;
  private statusTimer?: ReturnType<typeof setTimeout>;
  private heartbeat: ReturnType<typeof setInterval>;
  private lastHostSeen = Date.now();

  constructor(
    id: string,
    asHost: boolean,
    name: string,
    relays: string[],
    private readonly local: LocalGame,
  ) {
    this.id = id;
    this.code = roomCode(id);
    this.me = signal<Player>({ key: publicKey(this.secret), name, host: asHost, rules: SUPPORTED_RULES, lastSeen: Date.now(), me: true });
    this.pool = new RelayPool(relays, { kinds: [KIND], "#t": [roomTag(id)] }, (e) => this.receive(e));
    this.connected = this.pool.connected;
    this.relayCount = this.pool.total;
    this.heartbeat = setInterval(() => this.tick(), 1000);
    // Announce as soon as a relay is up.
    this.connected.subscribe((n) => n > 0 && untracked(() => this.sendStatus(true)));
  }

  setMode(mode: number) {
    this.mode.value = mode;
    this.sendStatus();
  }

  setName(name: string) {
    this.me.value = { ...this.me.value, name };
    this.sendStatus();
  }

  /** Host: start a new game for everybody. */
  start(mode: number, seed?: bigint) {
    if (!this.isHost.value || this.local.busy()) return;
    const s = seed ?? crypto.getRandomValues(new BigUint64Array(1))[0]!;
    const code = encodeText({ rules: RULES_VERSION, mode, seed: s });
    this.send({ type: "start", code, rules: RULES_VERSION, mode, seed: s });
    this.begin(code, mode, s);
  }

  /** Our own game's progress; call on every change. */
  report(game: GameState) {
    this.me.value = { ...this.me.value, game };
    this.sendStatus(game.over);
  }

  /** Join the newer game announced while we were playing. */
  joinPending() {
    const p = this.pending.value;
    if (p) this.begin(p.code, p.mode, p.seed);
  }

  leave() {
    this.send({ type: "bye" });
    clearInterval(this.heartbeat);
    clearTimeout(this.statusTimer);
    // Give the bye a moment to leave before closing the sockets.
    setTimeout(() => this.pool.close(), 300);
  }

  private begin(code: string, mode: number, seed: bigint) {
    this.played.add(code);
    this.pending.value = null;
    this.me.value = { ...this.me.value, game: { code, n: 1, score: 0, over: false } };
    this.local.play(code, mode, seed);
    this.sendStatus(true);
  }

  private offer(code: string, mode: number, seed: bigint) {
    if (this.played.has(code)) return;
    if (this.local.busy()) this.pending.value = { code, mode, seed };
    else this.begin(code, mode, seed);
  }

  private statusMessage(): Message {
    const me = this.me.value;
    return {
      type: "status",
      name: me.name,
      host: me.host,
      rules: me.rules,
      ...(this.isHost.value ? { mode: this.mode.value } : {}),
      ...(me.game ? { game: me.game } : {}),
    };
  }

  /** Sends our status now, or as soon as the minimum interval allows (`urgent` skips the wait). */
  private sendStatus(urgent = false) {
    const wait = this.lastSent + MIN_STATUS_INTERVAL_MS - Date.now();
    if (urgent || wait <= 0) {
      clearTimeout(this.statusTimer);
      this.statusTimer = undefined;
      this.send(this.statusMessage());
    } else if (!this.statusTimer) {
      this.statusTimer = setTimeout(() => {
        this.statusTimer = undefined;
        this.send(this.statusMessage());
      }, wait);
    }
  }

  private send(m: Message) {
    if (m.type === "status") this.lastSent = Date.now();
    this.pool.publish(createEvent(this.secret, Math.floor(Date.now() / 1000), this.id, encodeMessage(m)));
  }

  private tick() {
    const now = Date.now();
    if (now - this.lastSent >= HEARTBEAT_MS) this.sendStatus(true);
    this.remove((p) => now - p.lastSeen >= PRESENCE_TIMEOUT_MS);
    // No host for 15 s: the present player with the lowest key takes over (§4).
    if (this.hostKey.value !== null) this.lastHostSeen = now;
    else if (now - this.lastHostSeen >= PRESENCE_TIMEOUT_MS && this.everyone.value.map((p) => p.key).sort()[0] === this.me.value.key) {
      this.me.value = { ...this.me.value, host: true };
      this.sendStatus(true);
    }
  }

  private remove(gone: (p: Player) => boolean) {
    const leaving = this.players.value.filter(gone);
    if (!leaving.length) return;
    this.players.value = this.players.value.filter((p) => !gone(p));
    const keep = this.departed.value.filter((d) => !leaving.some((p) => p.key === d.key));
    this.departed.value = [...keep, ...leaving.filter((p) => p.game)];
  }

  private receive(e: NostrEvent) {
    if (e.pubkey === this.me.value.key) return;
    const m = parseMessage(e.content);
    if (!m) return;
    if (m.type === "bye") {
      this.remove((p) => p.key === e.pubkey);
      return;
    }
    const known = this.players.value.find((p) => p.key === e.pubkey);
    if (m.type === "start") {
      if (known) known.lastSeen = Date.now();
      if (e.pubkey === this.hostKey.value && SUPPORTED_RULES.includes(m.rules)) this.offer(m.code, m.mode, m.seed);
      return;
    }
    const player: Player = { key: e.pubkey, name: m.name, host: m.host, rules: m.rules, mode: m.mode, game: m.game, lastSeen: Date.now(), me: false };
    this.players.value = known ? this.players.value.map((p) => (p === known ? player : p)) : [...this.players.value, player];
    // A lower-keyed host wins (§4): stop claiming.
    if (this.me.value.host && !this.isHost.value) this.me.value = { ...this.me.value, host: false };
    if (e.pubkey === this.hostKey.value) {
      if (m.mode !== undefined && !this.isHost.value) this.mode.value = m.mode;
      // Late join: the host is playing a game we haven't played (§4).
      const g = m.game;
      if (g && !g.over) {
        const d = parseStartCode(g.code);
        if (d && SUPPORTED_RULES.includes(d.rules)) this.offer(g.code, d.mode, d.seed);
      }
    }
    if (!known) this.sendStatus(); // let the newcomer know us quickly
  }
}

function parseStartCode(code: string) {
  const m = parseMessage(JSON.stringify({ p: 1, type: "start", code }));
  return m?.type === "start" ? m : null;
}
