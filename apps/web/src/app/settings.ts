// SPDX-License-Identifier: GPL-3.0-or-later
// Persistent user settings (docs/design/product.md, "Settings").
import { computed, effect, signal } from "@preact/signals";
import { isRelayUrl, MAX_RELAYS } from "../net/multiplayer-v1.ts";
import { load, save } from "./storage.ts";

export const SUPPORTED_LANGUAGES = ["en", "de"] as const;
// docs/design/nostr-relays.md, list of 2026-10-25 (multiplayer v1 §3.4)
export const DEFAULT_RELAYS = ["wss://relay.damus.io", "wss://nos.lol", "wss://relay.primal.net"];
const MAX_NAME_LENGTH = 24;

function persisted<T>(key: string, fallback: T) {
  const s = signal<T>(load(key, fallback));
  effect(() => save(key, s.value));
  return s;
}

/** Removes control characters, trims and truncates to 24 code points (multiplayer v1 §2.1). */
export function cleanName(name: string): string {
  // eslint-disable-next-line no-control-regex
  return [...name.replace(/[\u0000-\u001f\u007f]/g, "").trim()].slice(0, MAX_NAME_LENGTH).join("");
}

/** "" = follow the system. */
export const language = persisted<string>("language", "");
export const colourBlind = persisted("colourBlind", false);
export const vibration = persisted("vibration", true);
// Chosen once, so the name stays the same until the user changes it.
export const playerName = persisted("playerName", `Player${100 + Math.floor(Math.random() * 900)}`);
/** Relay URLs separated by "\n"; "" = the default relays. */
export const relays = persisted("relays", "");

export const effectiveLanguage = computed(() => {
  if ((SUPPORTED_LANGUAGES as readonly string[]).includes(language.value)) return language.value;
  for (const tag of navigator.languages ?? [navigator.language]) {
    const code = tag.split("-")[0]!.toLowerCase();
    if ((SUPPORTED_LANGUAGES as readonly string[]).includes(code)) return code;
  }
  return "en";
});

/** Relays for a room this player creates: the valid ones from the setting (at most 8), else the defaults. */
export const roomRelays = computed(() => {
  const own = [...new Set(relays.value.split("\n").filter(isRelayUrl))].slice(0, MAX_RELAYS);
  return own.length ? own : DEFAULT_RELAYS;
});

export function setLanguage(code: string) {
  language.value = (SUPPORTED_LANGUAGES as readonly string[]).includes(code) ? code : "";
}

export function setPlayerName(name: string) {
  const clean = cleanName(name);
  if (clean) playerName.value = clean;
}

export function setRelays(text: string) {
  relays.value = text.split(/[\s,]+/).filter(Boolean).join("\n");
}

effect(() => {
  document.documentElement.lang = effectiveLanguage.value;
});
