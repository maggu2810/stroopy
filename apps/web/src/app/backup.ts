// SPDX-License-Identifier: GPL-3.0-or-later
// Backup export/import (docs/spec/backup/v1.md, docs/design/product.md "Backup").
import { encodeText, type Backup } from "../core/backup-v1.ts";
import { seedToHex } from "../core/codec.ts";
import { highscores, merge } from "./highscores.ts";
import { colourBlind, language, playerName, relays, setLanguage, setPlayerName, setRelays, SUPPORTED_LANGUAGES, vibration } from "./settings.ts";

/** All highscores (by mode, best first) and shared settings as a backup code. */
export function exportCode(): string {
  const entries = [...highscores.value]
    .sort((a, b) => a.mode - b.mode || b.score - a.score || a.time - b.time)
    .map((e) => ({ ...e, seed: BigInt("0x" + e.seed) }));
  return encodeText({
    entries,
    settings: { language: language.value, colourBlind: colourBlind.value, vibration: vibration.value, name: playerName.value, relays: relays.value },
  });
}

/** Merges the highscores and optionally applies the settings; returns the number of highscores added. */
export function importBackup(backup: Backup, restoreSettings: boolean): number {
  const added = merge(backup.entries.map((e) => ({ ...e, seed: seedToHex(e.seed) })));
  const s = backup.settings;
  if (restoreSettings && s) {
    // Unsupported values are ignored (e.g. a language this app doesn't have).
    if (s.language !== undefined && (s.language === "" || (SUPPORTED_LANGUAGES as readonly string[]).includes(s.language))) setLanguage(s.language);
    if (s.colourBlind !== undefined) colourBlind.value = s.colourBlind;
    if (s.vibration !== undefined) vibration.value = s.vibration;
    if (s.name !== undefined) setPlayerName(s.name);
    if (s.relays !== undefined) setRelays(s.relays);
  }
  return added;
}
