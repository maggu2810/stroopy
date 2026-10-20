// SPDX-License-Identifier: GPL-3.0-or-later
import { useEffect } from "preact/hooks";
import { Mode } from "../core/rules-v1.ts";
import { encodeText } from "../core/share-code-v1.ts";
import { controller } from "../app/controller.ts";
import { t } from "../app/i18n.ts";
import { focusFirst, keyHints, usePageInput } from "../app/input.ts";
import { modeName } from "../app/theme.ts";
import { shareText, toast } from "./common.tsx";

export function ResultPage(props: { rank: number; onReplay: () => void; onNewGame: () => void; onHome: () => void }) {
  const c = controller;
  const score = c.score.value;
  const mode = c.mode.value;
  const code = encodeText({ rules: c.rules, mode, seed: c.seed, result: { score, reached: c.reached } });
  const challenge = c.challenge.value;

  usePageInput({
    back: props.onHome,
    key: (e) => {
      if (e.key === "r" || e.key === "R") return props.onReplay(), true;
      if (e.key === "n" || e.key === "N") return props.onNewGame(), true;
      return false;
    },
  });
  useEffect(() => focusFirst(), []);

  async function share() {
    const result = await shareText(t("Can you beat %1 in Stroopy %2? %3", score, modeName(mode), code));
    if (result === "copied") toast(t("Message with code copied to the clipboard"));
  }

  return (
    <div class="page result" data-page>
      <div class="two-col">
        <div class="summary">
          <div class="muted big">{modeName(mode)}</div>
          <div class={`final-score ${props.rank === 1 ? "accent" : ""}`}>{score}</div>
          {props.rank > 0 && <div class="rank">{props.rank === 1 ? t("New best!") : t("Highscore #%1", props.rank)}</div>}
          <div class="muted big">{mode === Mode.Chrono ? t("time is up at #%1", c.reached) : t("out at #%1", c.reached)}</div>
          {challenge && (
            <div class={`verdict ${score > challenge.score ? "good" : score < challenge.score ? "bad" : ""}`}>
              {score > challenge.score
                ? t("You beat %1!", challenge.score)
                : score < challenge.score
                  ? t("%1 to beat, try again!", challenge.score)
                  : t("A draw at %1!", challenge.score)}
            </div>
          )}
        </div>
        <div class="actions">
          <input class="code" readOnly value={code} tabIndex={-1} />
          <button onClick={share}>{t("Share code")}</button>
          <button class="gap" onClick={props.onReplay}>
            {t("Play this sequence again")}
            {keyHints.value && "  [R]"}
          </button>
          <button class="primary" data-autofocus onClick={props.onNewGame}>
            {t("New game")}
            {keyHints.value && "  [N]"}
          </button>
          <button class="flat" onClick={props.onHome}>
            {t("Home")}
          </button>
        </div>
      </div>
    </div>
  );
}
