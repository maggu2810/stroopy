// SPDX-License-Identifier: GPL-3.0-or-later
import { vibration } from "./settings.ts";

/** Short vibration, if enabled and supported (not on iOS). */
export function vibrate(ms: number) {
  if (vibration.value) navigator.vibrate?.(ms);
}
