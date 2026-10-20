// SPDX-License-Identifier: GPL-3.0-or-later
// Every translatable string (t("…") literals and the colour words) has a German translation.
import { expect, test } from "bun:test";
import { Glob } from "bun";
import { join } from "node:path";
import { de } from "../src/app/i18n-de.ts";

test("German translations are complete", async () => {
  const sources = new Set(["YELLOW", "RED", "BLUE", "GREEN", "ORANGE", "PURPLE", "PINK", "BROWN", "CYAN", "BEIGE"]);
  const src = join(import.meta.dir, "../src");
  for await (const file of new Glob("**/*.{ts,tsx}").scan(src)) {
    const text = await Bun.file(join(src, file)).text();
    for (const m of text.matchAll(/\bt\(\s*"((?:[^"\\]|\\.)*)"/g)) sources.add(JSON.parse(`"${m[1]}"`));
  }
  const missing = [...sources].filter((s) => !(s in de));
  expect(missing).toEqual([]);
});
