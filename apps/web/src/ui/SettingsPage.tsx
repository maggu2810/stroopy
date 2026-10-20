// SPDX-License-Identifier: GPL-3.0-or-later
import { clear } from "../app/highscores.ts";
import { t } from "../app/i18n.ts";
import { pop, push } from "../app/nav.ts";
import { colourBlind, DEFAULT_RELAYS, language, playerName, relays, setLanguage, setPlayerName, setRelays, vibration } from "../app/settings.ts";
import { SubPage } from "./common.tsx";

export function SettingsPage() {
  return (
    <SubPage title={t("Settings")} onBack={pop} class="settings">
      <div class="column">
        <label class="field">
          <span class="muted">{t("Language")}</span>
          {/* Language names are shown in their own language. */}
          <select data-autofocus value={language.value} onChange={(e) => setLanguage(e.currentTarget.value)}>
            <option value="">{t("Follow system")}</option>
            <option value="en">English</option>
            <option value="de">Deutsch</option>
          </select>
        </label>
        <label class="switch">
          <input type="checkbox" checked={colourBlind.value} onChange={(e) => (colourBlind.value = e.currentTarget.checked)} />
          <span>{t("Colour-blind palette")}</span>
        </label>
        <label class="switch">
          <input type="checkbox" checked={vibration.value} onChange={(e) => (vibration.value = e.currentTarget.checked)} />
          <span>{t("Vibrate on failure")}</span>
        </label>
        <label class="field">
          <span class="muted">{t("Player name")}</span>
          <input type="text" maxLength={24} value={playerName.value} onChange={(e) => setPlayerName(e.currentTarget.value)} />
          <small class="muted">{t("Shown to other players. Names and scores travel unencrypted through public Nostr relays.")}</small>
        </label>
        <label class="field">
          <span class="muted">{t("Nostr relays (separated by spaces, empty = default)")}</span>
          <input
            type="text"
            inputMode="url"
            spellcheck={false}
            autocapitalize="off"
            value={relays.value.split("\n").join(" ")}
            placeholder={DEFAULT_RELAYS.join(" ")}
            onChange={(e) => setRelays(e.currentTarget.value)}
          />
        </label>
        <button class="gap" onClick={() => push({ name: "backup" })}>
          {t("Backup: export / import")}
        </button>
        <button
          class="flat bad"
          onClick={() => {
            if (confirm(t("Clear all highscores?"))) clear();
          }}
        >
          {t("Clear highscores")}
        </button>
      </div>
    </SubPage>
  );
}
