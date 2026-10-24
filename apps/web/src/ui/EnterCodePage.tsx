// SPDX-License-Identifier: GPL-3.0-or-later
import { useState } from "preact/hooks";
import { modeFromId, RULES_VERSION } from "../core/rules-v1.ts";
import { decodeText } from "../core/share-code-v1.ts";
import { t } from "../app/i18n.ts";
import { pop } from "../app/nav.ts";
import { modeName } from "../app/theme.ts";
import { pasteText, SubPage } from "./common.tsx";

export function EnterCodePage(props: { text?: string; onPlay: (mode: number, seed: bigint, result?: { score: number; reached: number }) => void }) {
  const [text, setText] = useState(props.text ?? "");
  const d = decodeText(text);
  const playable = d.ok && d.value.rules === RULES_VERSION && modeFromId(d.value.mode) !== undefined;

  let status = "";
  if (!d.ok) status = d.kind === "unsupported" ? t("This code needs a newer version of Stroopy.") : t("No valid code found.");
  else if (!playable) status = t("This code needs a newer version of Stroopy (rules %1).", d.value.rules);
  else {
    status = modeName(d.value.mode);
    if (d.value.result) status += "\n" + t("Score to beat: %1 (out at #%2)", d.value.result.score, d.value.result.reached);
  }
  const play = () => d.ok && playable && props.onPlay(d.value.mode, d.value.seed, d.value.result);

  return (
    <SubPage title={t("Enter code")} onBack={pop} class="enter-code">
      <div class="column">
        <p class="muted">{t("Paste a code or a whole message containing one.")}</p>
        <textarea
          data-autofocus
          rows={4}
          placeholder="SYG1-…"
          spellcheck={false}
          autocapitalize="off"
          value={text}
          onInput={(e) => setText(e.currentTarget.value)}
          onKeyDown={(e) => {
            if (e.key === "Enter" && playable) {
              e.preventDefault();
              play();
            }
          }}
        />
        <button class="inline" onClick={async () => setText((await pasteText()) ?? text)}>
          {t("Paste")}
        </button>
        {text && <p class={`status ${playable ? "" : "bad"}`}>{status}</p>}
        <button class="primary tall" disabled={!playable} onClick={play}>
          {t("Play")}
        </button>
      </div>
    </SubPage>
  );
}
