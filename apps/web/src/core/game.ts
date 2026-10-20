// SPDX-License-Identifier: GPL-3.0-or-later
// Game session per docs/spec/rules/v1.md §6: answers, scoring and end of game.
// Time is supplied by the caller, so the logic stays deterministic and testable.
import {
  CHRONO_TOTAL_MS,
  itemLimitMs,
  Mode,
  roundLimitMs,
  TapRoundGenerator,
  WordItemGenerator,
  type TapRound,
  type WordItem,
} from "./rules-v1.ts";

export type Outcome = "correct" | "wrong" | "over";

export class Game {
  private words: WordItemGenerator;
  private rounds: TapRoundGenerator;
  current: WordItem | TapRound;
  score = 0;
  over = false;

  constructor(
    readonly mode: Mode,
    readonly seed: bigint,
  ) {
    this.words = new WordItemGenerator(mode, seed);
    this.rounds = new TapRoundGenerator(seed);
    this.current = this.advance();
  }

  get n(): number {
    return this.current.n;
  }

  /** Number of the item/round that failed or was unanswered at the end (spec §6). */
  get reached(): number {
    return this.current.n;
  }

  /** Limit for the current item/round in ms; for chrono the total game limit. */
  get limitMs(): number {
    if (this.mode === Mode.Chrono) return CHRONO_TOTAL_MS;
    if (this.mode === Mode.Tap) return roundLimitMs(this.n);
    return itemLimitMs(this.n);
  }

  /**
   * choice: classic/chrono 1 = TRUE, 0 = FALSE; find: colour slot; tap: card position.
   * elapsedMs: since the current item was shown (ignored for chrono).
   */
  answer(choice: number, elapsedMs: number): Outcome {
    if (this.over) return "over";
    if (this.mode !== Mode.Chrono && elapsedMs >= this.limitMs) {
      this.over = true;
      return "over";
    }
    const correct = this.isCorrect(choice);
    if (this.mode === Mode.Chrono) {
      this.score = correct ? this.score + 1 : Math.max(0, this.score - 1);
      this.current = this.advance();
      return correct ? "correct" : "wrong";
    }
    if (!correct) {
      this.over = true;
      return "over";
    }
    ++this.score;
    this.current = this.advance();
    return "correct";
  }

  /** The current limit (or chrono's total limit) has been reached. */
  timeout(): void {
    this.over = true;
  }

  private isCorrect(choice: number): boolean {
    const c = this.current;
    if (c.kind === "tap") return choice === c.trueCard;
    if (this.mode === Mode.Find) return choice === c.word;
    return (choice === 1) === (c.ink === c.word);
  }

  private advance(): WordItem | TapRound {
    return this.mode === Mode.Tap ? this.rounds.next() : this.words.next();
  }
}
