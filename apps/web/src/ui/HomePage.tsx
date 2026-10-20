// SPDX-License-Identifier: GPL-3.0-or-later
import { useEffect } from "preact/hooks";
import { MODES, Mode } from "../core/rules-v1.ts";
import { best, highscores } from "../app/highscores.ts";
import { t } from "../app/i18n.ts";
import { focusFirst, usePageInput } from "../app/input.ts";
import { push } from "../app/nav.ts";
import { ink, modeName } from "../app/theme.ts";

export function HomePage(props: { onPlay: (mode: Mode) => void }) {
  usePageInput({});
  useEffect(() => focusFirst(), []);
  void highscores.value; // re-render when highscores change
  const hints: Record<Mode, string> = {
    [Mode.Classic]: t("True or false?"),
    [Mode.Chrono]: t("30 seconds, as many as you can"),
    [Mode.Find]: t("Tap the colour the word names"),
    [Mode.Tap]: t("Only one card is right"),
  };
  return (
    <div class="page home" data-page>
      <div class="home-grid">
        <div class="brand">
          <h1 style={{ color: ink(2) }}>STROOPY</h1>
          <p>{t("Does the colour match the word?")}</p>
        </div>
        <div class="menu">
          <div class="modes">
            {MODES.map((mode, i) => (
              <button class="mode" style={{ background: ink(i) }} data-autofocus={i === 0 || undefined} onClick={() => props.onPlay(mode)}>
                <span class="mode-text">
                  <b>{modeName(mode)}</b>
                  <small>{hints[mode]}</small>
                </span>
                {best(mode) > 0 && <span class="best">★ {best(mode)}</span>}
              </button>
            ))}
          </div>
          <div class="links">
            <button class="flat" onClick={() => push({ name: "highscores" })}>{t("Highscores")}</button>
            <button class="flat" onClick={() => push({ name: "enterCode" })}>{t("Enter code")}</button>
            <button class="flat" onClick={() => push({ name: "help" })}>{t("Help")}</button>
            <button class="flat" onClick={() => push({ name: "settings" })}>{t("Settings")}</button>
          </div>
        </div>
      </div>
    </div>
  );
}
