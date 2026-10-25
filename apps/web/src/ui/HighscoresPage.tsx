// SPDX-License-Identifier: GPL-3.0-or-later
import { useState } from "preact/hooks";
import { MODES, RULES_VERSION } from "../core/rules-v1.ts";
import { encodeText } from "../core/game-v1.ts";
import { entries, highscores } from "../app/highscores.ts";
import { t } from "../app/i18n.ts";
import { effectiveLanguage } from "../app/settings.ts";
import { modeName } from "../app/theme.ts";
import { pop } from "../app/nav.ts";
import { shareText, SubPage, toast } from "./common.tsx";

export function HighscoresPage(props: { onPlay: (mode: number, seed: bigint) => void }) {
  const [mode, setMode] = useState<number>(MODES[0]!);
  void highscores.value;
  const list = entries(mode);
  const date = new Intl.DateTimeFormat(effectiveLanguage.value, { dateStyle: "short", timeStyle: "short" });

  return (
    <SubPage title={t("Highscores")} onBack={pop} class="highscores">
      <div class="tabs" role="tablist">
        {MODES.map((m, i) => (
          <button role="tab" aria-selected={m === mode} class={m === mode ? "active" : ""} data-autofocus={i === 0 || undefined} onClick={() => setMode(m)}>
            {modeName(m)}
          </button>
        ))}
      </div>
      {list.length === 0 && <p class="empty muted">{t("No highscores yet")}</p>}
      <ol class="scores">
        {list.map((e, i) => (
          <li>
            <span class="pos muted">{i + 1}</span>
            <span class={`points ${i === 0 ? "accent" : ""}`}>{e.score}</span>
            <span class="details">
              <span>{e.challenge ? t("challenge · out at #%1", e.reached) : t("out at #%1", e.reached)}</span>
              <small class="muted">{date.format(new Date(e.time * 1000))}</small>
            </span>
            <button
              class="flat"
              onClick={async () => {
                const code = encodeText({ rules: e.rules, mode: e.mode, seed: BigInt("0x" + e.seed), result: { score: e.score, reached: e.reached } });
                if ((await shareText(code)) === "copied") toast(t("Code copied to the clipboard"));
              }}
            >
              {t("Share")}
            </button>
            <button class="flat" aria-label={t("Play")} disabled={e.rules !== RULES_VERSION} onClick={() => props.onPlay(e.mode, BigInt("0x" + e.seed))}>
              ▶
            </button>
          </li>
        ))}
      </ol>
    </SubPage>
  );
}
