// SPDX-License-Identifier: GPL-3.0-or-later
// Play view for all modes. Layouts and bindings: docs/design/product.md, "Layout" and "Input".
import type { ComponentChildren } from "preact";
import { useEffect } from "preact/hooks";
import { Mode } from "../core/rules-v1.ts";
import { controller } from "../app/controller.ts";
import { vibrate } from "../app/haptics.ts";
import { t } from "../app/i18n.ts";
import { Pad, shoulderHeld, usePageInput } from "../app/input.ts";
import { ink, word } from "../app/theme.ts";

// Diamond directions: 0 up, 1 right, 2 down, 3 left.
// Find: inner slots top 0, right 1, bottom 3, left 2 (d ^ (d >> 1), self-inverse);
// outer slots 4..7 clockwise from top right.
const innerSlot = (d: number) => d ^ (d >> 1);
const ARROWS = "↑→↓←";
const FIND_CELLS = [7, 0, 4, 2, -1, 1, 6, 3, 5];
const TAP_CELLS = [-1, 0, -1, 3, -1, 1, -1, 2, -1];
const TAP_HINTS = ["↑ W", "→ D", "↓ S", "← A"];
const KEY_DIRECTIONS: Record<string, number> = {
  ArrowUp: 0, w: 0, W: 0, ArrowRight: 1, d: 1, D: 1, ArrowDown: 2, s: 2, S: 2, ArrowLeft: 3, a: 3, A: 3,
};

export function GamePage(props: { onQuit: () => void }) {
  const c = controller;
  const mode = c.mode.value;
  const phase = c.phase.value;
  const current = c.current.value;
  const running = phase === "running" && current !== null;
  const trueFalse = mode === Mode.Classic || mode === Mode.Chrono;
  const pool = current?.pool ?? 4;

  function direction(d: number, outer: boolean) {
    if (mode === Mode.Tap) c.answer(d);
    else if (mode === Mode.Find) {
      const slot = outer ? 4 + d : innerSlot(d);
      if (slot < pool) c.answer(slot);
    } else if (d === 1 || d === 3) c.answer(d === 1 ? 1 : 0);
  }

  usePageInput({
    back: props.onQuit,
    key: (e) => {
      if (e.key === "Escape") return props.onQuit(), true;
      if (!running) return true;
      const d = KEY_DIRECTIONS[e.key];
      if (d !== undefined) return direction(d, e.shiftKey || shoulderHeld), true;
      if (trueFalse && (e.key === "f" || e.key === "F")) return c.answer(0), true;
      if (trueFalse && (e.key === "j" || e.key === "J")) return c.answer(1), true;
      const digit = Number(e.key) - 1;
      if (mode === Mode.Find && digit >= 0 && digit < pool) return c.answer(digit), true;
      return e.key !== "Tab";
    },
    pad: (b) => {
      if (b === Pad.Start || b === Pad.Select) return props.onQuit(), true;
      if (!running) return true;
      if (trueFalse && (b === Pad.B || b === Pad.A)) return c.answer(b === Pad.A ? 1 : 0), true;
      const d = ({ [Pad.Y]: 0, [Pad.B]: 1, [Pad.A]: 2, [Pad.X]: 3, [Pad.Up]: 0, [Pad.Right]: 1, [Pad.Down]: 2, [Pad.Left]: 3 } as Record<number, number>)[b];
      if (d !== undefined) direction(d, shoulderHeld);
      return true;
    },
  });

  // Feedback on answers: flash, and vibration on failure.
  const answers = c.answers.value;
  useEffect(() => {
    if (answers > 0 && !c.lastCorrect) vibrate(80);
  }, [answers]);
  useEffect(() => {
    if (phase === "over") vibrate(120);
  }, [phase]);
  useEffect(() => {
    (document.activeElement as HTMLElement | null)?.blur();
  }, []);

  const word_ = current?.kind === "word" ? current : null;
  const tap = current?.kind === "tap" ? current : null;

  return (
    <div class={`page game mode-${mode}`} data-page>
      {answers > 0 && <div key={answers} class="flash" style={{ background: c.lastCorrect ? "var(--good)" : "var(--bad)" }} />}
      <header class="game-bar">
        <button class="icon" tabIndex={-1} aria-label="Quit" onClick={props.onQuit}>
          ✕
        </button>
        <span class="spacer" />
        <span class="n">#{c.n}</span>
        <span class="score">{c.score.value}</span>
      </header>
      {(mode === Mode.Chrono || mode === Mode.Tap) && (
        <div class="timebar">
          {running && <div key={mode === Mode.Tap ? current.n : "chrono"} class="fill" style={{ animationDuration: `${c.limitMs}ms` }} />}
        </div>
      )}
      {phase === "countdown" && <div class="countdown">{c.countdown.value > 0 ? c.countdown.value : t("GO")}</div>}
      {running && (
        <div class="play">
          {word_ && (
            <div class="challenge">
              <div class="square">
                {mode !== Mode.Chrono && <Ring key={word_.n} color={ink(word_.ink)} durationMs={c.limitMs} width={Math.max(2, 7 * (1 - word_.n / 300))} />}
                <span class="word" style={{ color: ink(word_.ink) }}>
                  {word(word_.word)}
                </span>
              </div>
            </div>
          )}
          {trueFalse && (
            <div class="answers tf">
              <AnswerButton fill="var(--bad)" hint="← F" onClick={() => c.answer(0)}>
                {t("FALSE")}
              </AnswerButton>
              <AnswerButton fill="var(--good)" hint="J →" onClick={() => c.answer(1)}>
                {t("TRUE")}
              </AnswerButton>
            </div>
          )}
          {mode === Mode.Find && (
            <Diamond cells={FIND_CELLS}>
              {(slot) =>
                slot < pool && (
                  <AnswerButton fill={ink(slot)} hint={`${slot + 1}  ${slot < 4 ? ARROWS[innerSlot(slot)] : "⇧" + ARROWS[slot - 4]}`} onClick={() => c.answer(slot)} />
                )
              }
            </Diamond>
          )}
          {tap && (
            <Diamond cells={TAP_CELLS} wide>
              {(slot) => (
                <button class="card" tabIndex={-1} onPointerDown={(e) => (e.preventDefault(), c.answer(slot))}>
                  <span class="card-word" style={{ color: ink(tap.inks[slot]!) }}>
                    {word(tap.words[slot]!)}
                  </span>
                  <span class="hint">{TAP_HINTS[slot]}</span>
                </button>
              )}
            </Diamond>
          )}
        </div>
      )}
    </div>
  );
}

function AnswerButton(props: { fill: string; hint: string; onClick: () => void; children?: ComponentChildren }) {
  // pointerdown: answers count from the touch, not from the release (speed matters).
  return (
    <button class="answer" tabIndex={-1} style={{ background: props.fill }} onPointerDown={(e) => (e.preventDefault(), props.onClick())}>
      {props.children}
      <span class="hint">{props.hint}</span>
    </button>
  );
}

/** 3x3 grid, square, centred; `cells` maps each cell to a slot (-1 = empty). */
function Diamond(props: { cells: number[]; wide?: boolean; children: (slot: number) => ComponentChildren }) {
  return (
    <div class={`diamond-box ${props.wide ? "wide" : ""}`}>
      <div class="diamond">{props.cells.map((slot, i) => <div key={i}>{slot >= 0 && props.children(slot)}</div>)}</div>
    </div>
  );
}

/** Circular timer that drains from full to empty over `durationMs`. Re-mount (key) to restart. */
function Ring(props: { color: string; durationMs: number; width: number }) {
  const r = 50 - props.width / 2;
  return (
    <svg class="ring" viewBox="0 0 100 100" aria-hidden="true">
      <circle cx="50" cy="50" r={r} fill="none" stroke={props.color} stroke-opacity="0.15" stroke-width={props.width} />
      <circle
        class="drain"
        cx="50"
        cy="50"
        r={r}
        fill="none"
        stroke={props.color}
        stroke-width={props.width}
        stroke-linecap="round"
        pathLength={100}
        style={{ animationDuration: `${props.durationMs}ms` }}
      />
    </svg>
  );
}
