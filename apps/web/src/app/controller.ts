// SPDX-License-Identifier: GPL-3.0-or-later
// Runs one game: countdown, timers and answers (docs/spec/rules/v1.md, appendix A "suggested feel").
import { batch, signal } from "@preact/signals";
import { Game } from "../core/game.ts";
import { CHRONO_TOTAL_MS, Mode, RULES_VERSION, type TapRound, type WordItem } from "../core/rules-v1.ts";
import { seedToHex } from "../core/codec.ts";

export type Phase = "idle" | "countdown" | "running" | "over";

const COUNTDOWN_STEP_MS = 1000;

function randomSeed(): bigint {
  const words = crypto.getRandomValues(new Uint32Array(2));
  return (BigInt(words[0]!) << 32n) | BigInt(words[1]!);
}

class Controller {
  readonly phase = signal<Phase>("idle");
  readonly countdown = signal(3);
  readonly mode = signal<Mode>(Mode.Classic);
  readonly current = signal<WordItem | TapRound | null>(null);
  readonly score = signal(0);
  /** Increments on every answer; `lastCorrect` tells how it went. */
  readonly answers = signal(0);
  lastCorrect = true;
  /** Set when the game was started from a game code with a result. */
  readonly challenge = signal<{ score: number; reached: number } | null>(null);
  readonly isChallenge = signal(false);

  private game: Game | null = null;
  private shownAt = 0;
  private countdownTimer: ReturnType<typeof setInterval> | undefined;
  private limitTimer: ReturnType<typeof setTimeout> | undefined;
  private overListeners: (() => void)[] = [];

  get rules() {
    return RULES_VERSION;
  }
  get seed(): bigint {
    return this.game?.seed ?? 0n;
  }
  get seedHex(): string {
    return seedToHex(this.seed);
  }
  get reached(): number {
    return this.game?.reached ?? 0;
  }
  get limitMs(): number {
    return this.game?.limitMs ?? 0;
  }
  get n(): number {
    return this.game?.n ?? 0;
  }

  onOver(listener: () => void) {
    this.overListeners.push(listener);
  }

  start(mode: Mode, seed?: bigint, challenge?: { score: number; reached: number } | null, isChallenge = false) {
    this.abort();
    this.game = new Game(mode, seed ?? randomSeed());
    batch(() => {
      this.mode.value = mode;
      this.challenge.value = challenge ?? null;
      this.isChallenge.value = isChallenge;
      this.score.value = 0;
      this.current.value = null;
      this.countdown.value = 3;
      this.phase.value = "countdown";
    });
    this.countdownTimer = setInterval(() => this.tick(), COUNTDOWN_STEP_MS);
  }

  restart() {
    if (this.game) this.start(this.game.mode, this.game.seed, this.challenge.value, this.isChallenge.value);
  }

  answer(choice: number) {
    if (this.phase.value !== "running" || !this.game) return;
    const outcome = this.game.answer(choice, performance.now() - this.shownAt);
    this.lastCorrect = outcome === "correct";
    batch(() => {
      this.score.value = this.game!.score;
      this.answers.value++;
    });
    if (this.game.over) this.finish();
    else this.show();
  }

  abort() {
    clearInterval(this.countdownTimer);
    clearTimeout(this.limitTimer);
    this.phase.value = "idle";
  }

  private tick() {
    if (--this.countdown.value >= 0) return;
    clearInterval(this.countdownTimer);
    this.phase.value = "running";
    if (this.mode.value === Mode.Chrono) this.limitTimer = setTimeout(() => this.timeout(), CHRONO_TOTAL_MS);
    this.show();
  }

  private show() {
    const game = this.game!;
    this.current.value = game.current;
    this.shownAt = performance.now();
    if (game.mode !== Mode.Chrono) {
      clearTimeout(this.limitTimer);
      this.limitTimer = setTimeout(() => this.timeout(), game.limitMs);
    }
  }

  private timeout() {
    this.game?.timeout();
    this.finish();
  }

  private finish() {
    clearTimeout(this.limitTimer);
    this.phase.value = "over";
    for (const l of this.overListeners) l();
  }
}

export const controller = new Controller();
