// SPDX-License-Identifier: GPL-3.0-or-later
// Presentation of colour slots (docs/spec/rules/v1.md, appendix A).
import { Mode } from "../core/rules-v1.ts";
import { t } from "./i18n.ts";
import { colourBlind } from "./settings.ts";

const normalInks = ["#f1c40f", "#e74c3c", "#3498db", "#2ecc71", "#e67e22", "#9b59b6", "#ff89e5", "#a67235"];
const colourBlindInks = ["#ffd71d", "#f95341", "#0a88dc", "#18df6c", "#04e5e3", "#cfb891", "#8b0eaa", "#7d4f1b"];
const normalWords = ["YELLOW", "RED", "BLUE", "GREEN", "ORANGE", "PURPLE", "PINK", "BROWN"];
const colourBlindWords = ["YELLOW", "RED", "BLUE", "GREEN", "CYAN", "BEIGE", "PURPLE", "BROWN"];

export function ink(slot: number): string {
  return (colourBlind.value ? colourBlindInks : normalInks)[slot] ?? "#fff";
}

export function word(slot: number): string {
  return t((colourBlind.value ? colourBlindWords : normalWords)[slot] ?? "");
}

export function modeName(mode: number): string {
  switch (mode) {
    case Mode.Classic:
      return t("Classic");
    case Mode.Chrono:
      return t("Chrono");
    case Mode.Find:
      return t("Find the colour");
    case Mode.Tap:
      return t("Tap the true one");
  }
  return t("Unknown mode %1", mode);
}
