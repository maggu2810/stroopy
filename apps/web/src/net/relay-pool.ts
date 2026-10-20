// SPDX-License-Identifier: GPL-3.0-or-later
// Connections to several Nostr relays (docs/spec/protocol/v1.md §3.3): publish to all,
// subscribe on all, deduplicate and verify incoming events, reconnect with backoff.
import { signal } from "@preact/signals";
import { verifyEvent, type NostrEvent } from "./protocol-v1.ts";

const MAX_BACKOFF_MS = 30000;
const SEEN_LIMIT = 2000;

interface Relay {
  url: string;
  socket: WebSocket | null;
  backoffMs: number;
  timer?: ReturnType<typeof setTimeout>;
}

export class RelayPool {
  /** Number of relays with an open connection. */
  readonly connected = signal(0);
  private relays: Relay[];
  private seen = new Set<string>();
  private closed = false;
  private readonly subscription = "stroopy-" + Math.random().toString(36).slice(2, 10);

  constructor(
    urls: string[],
    private readonly filter: Record<string, unknown>,
    private readonly onEvent: (event: NostrEvent) => void,
  ) {
    this.relays = urls.map((url) => ({ url, socket: null, backoffMs: 1000 }));
    for (const r of this.relays) this.connect(r);
  }

  get total() {
    return this.relays.length;
  }

  publish(event: NostrEvent) {
    this.seen.add(event.id);
    const frame = JSON.stringify(["EVENT", event]);
    for (const r of this.relays) if (r.socket?.readyState === WebSocket.OPEN) r.socket.send(frame);
  }

  close() {
    this.closed = true;
    for (const r of this.relays) {
      clearTimeout(r.timer);
      r.socket?.close();
    }
    this.connected.value = 0;
  }

  private update() {
    this.connected.value = this.relays.filter((r) => r.socket?.readyState === WebSocket.OPEN).length;
  }

  private connect(r: Relay) {
    let socket: WebSocket;
    try {
      socket = new WebSocket(r.url);
    } catch {
      return this.retry(r); // malformed URL: keep trying, the list may be fixed in the settings
    }
    r.socket = socket;
    socket.onopen = () => {
      r.backoffMs = 1000;
      socket.send(JSON.stringify(["REQ", this.subscription, this.filter]));
      this.update();
    };
    socket.onmessage = (m) => this.receive(m.data);
    socket.onclose = () => {
      if (r.socket === socket) r.socket = null;
      this.update();
      this.retry(r);
    };
    socket.onerror = () => socket.close();
  }

  private retry(r: Relay) {
    if (this.closed) return;
    clearTimeout(r.timer);
    r.timer = setTimeout(() => this.connect(r), r.backoffMs);
    r.backoffMs = Math.min(r.backoffMs * 2, MAX_BACKOFF_MS);
  }

  private receive(data: unknown) {
    let frame: unknown;
    try {
      frame = JSON.parse(String(data));
    } catch {
      return;
    }
    // OK, EOSE, NOTICE and CLOSED are informative only.
    if (!Array.isArray(frame) || frame[0] !== "EVENT" || frame[1] !== this.subscription) return;
    const event = frame[2] as NostrEvent;
    if (typeof event?.id !== "string" || this.seen.has(event.id)) return;
    if (!verifyEvent(event)) return;
    this.seen.add(event.id);
    if (this.seen.size > SEEN_LIMIT) this.seen = new Set([...this.seen].slice(-SEEN_LIMIT / 2));
    this.onEvent(event);
  }
}
