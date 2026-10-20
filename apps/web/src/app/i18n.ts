// SPDX-License-Identifier: GPL-3.0-or-later
// English source strings, translated by lookup. Reading the language inside t() makes
// components re-render when it changes (docs/design/product.md, "Languages").
import { de } from "./i18n-de.ts";
import { effectiveLanguage } from "./settings.ts";

const tables: Record<string, Record<string, string>> = { de };

/** Translates `source` and replaces %1, %2, … with the arguments. */
export function t(source: string, ...args: (string | number)[]): string {
  const text = tables[effectiveLanguage.value]?.[source] ?? source;
  return args.length ? text.replace(/%(\d)/g, (_, i: string) => String(args[Number(i) - 1] ?? "")) : text;
}
