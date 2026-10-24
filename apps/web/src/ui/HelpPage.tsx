// SPDX-License-Identifier: GPL-3.0-or-later
// In-app documentation (docs/design/product.md, "Help").
import type { ComponentChildren } from "preact";
import { t } from "../app/i18n.ts";
import { pop } from "../app/nav.ts";
import { modeName } from "../app/theme.ts";
import { Mode } from "../core/rules-v1.ts";
import { SubPage } from "./common.tsx";

import pkg from "../../package.json";

function Entry(props: { term: string; children: ComponentChildren }) {
  return (
    <>
      <dt>{props.term}</dt>
      <dd>{props.children}</dd>
    </>
  );
}

export function HelpPage() {
  return (
    <SubPage title={t("Help")} onBack={pop} class="help">
      {/* Focusable, so arrow keys and a remote can scroll it. */}
      <article class="column prose" tabIndex={0} data-autofocus>
        <h3>{t("The idea")}</h3>
        <p>
          {t("A colour word is shown in an ink colour. Your brain reads the word faster than it names the ink – that conflict is the Stroop effect. Be quick and precise: the game gets faster, and from item 20 on more colours join in.")}
        </p>
        <h3>{t("Modes")}</h3>
        <dl>
          <Entry term={modeName(Mode.Classic)}>{t("Does the ink match the word? Answer TRUE or FALSE before the ring runs out. One mistake ends the game.")}</Entry>
          <Entry term={modeName(Mode.Chrono)}>{t("TRUE or FALSE for 30 seconds. +1 for a right answer, −1 for a wrong one.")}</Entry>
          <Entry term={modeName(Mode.Find)}>{t("The ink never matches. Tap the colour that the word names, not the ink. One mistake ends the game.")}</Entry>
          <Entry term={modeName(Mode.Tap)}>{t("Four cards, exactly one of them shows its word in the matching ink. Find it before the bar runs out.")}</Entry>
        </dl>
        <h3>{t("Keyboard")}</h3>
        <dl class="keys">
          <Entry term="← / F    → / J">{t("FALSE / TRUE (Classic, Chrono)")}</Entry>
          <Entry term="↑ → ↓ ←   W D S A">{t("Card at that position (Tap)")}</Entry>
          <Entry term="↑ → ↓ ←">{t("Inner colours: yellow, red, green, blue (Find)")}</Entry>
          <Entry term="⇧ + ↑ → ↓ ←">{t("Outer colours, clockwise from top right (Find)")}</Entry>
          <Entry term="1 – 8">{t("Colour by its number on the button (Find)")}</Entry>
          <Entry term={t("Arrows, Tab")}>{t("Move between buttons in menus")}</Entry>
          <Entry term={t("Enter, Space")}>{t("Press the selected button")}</Entry>
          <Entry term="Esc">{t("Back; quit a running game")}</Entry>
          <Entry term="R / N">{t("Replay / new game on the result screen")}</Entry>
          <Entry term="F11">{t("Fullscreen")}</Entry>
        </dl>
        <h3>{t("Gamepad")}</h3>
        <dl class="keys">
          <Entry term="B / A">{t("FALSE / TRUE (Classic, Chrono)")}</Entry>
          <Entry term={t("Y B A X or D-pad")}>{t("Top, right, bottom, left (Tap, Find)")}</Entry>
          <Entry term={t("L1/R1 + direction")}>{t("Outer colours (Find)")}</Entry>
          <Entry term={t("A / B in menus")}>{t("Press / back")}</Entry>
          <Entry term={t("Start, Select")}>{t("Quit a running game")}</Entry>
        </dl>
        <h3>{t("Sharing and challenges")}</h3>
        <p>
          {t("Every game is a sequence that can be replayed exactly. After a game, or for any highscore, copy the code (for example SYG1-…) and send it to a friend. They paste it under “Enter code” – even a whole message works – and play the very same sequence against your score.")}
        </p>
        <h3>{t("Multiplayer")}</h3>
        <p>{t("Create a room and send its code (SYR1-…) to your friends; they join under “Multiplayer”. The host picks the mode and starts, then everyone plays the same sequence at the same time and sees the others’ progress. Rooms run over public Nostr relays: no account, no server of ours, but names and scores are not encrypted.")}</p>
        <h3>{t("Install")}</h3>
        <p>{t("Stroopy runs in the browser and works offline after the first visit. Add it to your home screen (iPhone: Share → Add to Home Screen) to start it like an app. Browsers may delete website data; keep a backup code (Settings → Backup).")}</p>
        <h3>{t("About")}</h3>
        <dl>
          <Entry term={t("Version")}>{pkg.version}</Entry>
          <Entry term={t("Compatibility")}>{t("rules 1 · share code 1 · backup 1 · protocol 1")}</Entry>
        </dl>
        <p>
          {t("Stroopy is free software under the GNU General Public License, version 3 or later. It contains no ads and no tracking, and it works without Google services.")}
        </p>
      </article>
    </SubPage>
  );
}
