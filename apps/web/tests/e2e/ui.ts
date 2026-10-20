// SPDX-License-Identifier: GPL-3.0-or-later
// Browser UI test: builds the app, serves dist/ and drives it with key presses in headless
// Chromium, like apps/qt/tests/tst_ui.cpp. Usage: bun run test:ui [screenshot-dir]
// CHROME=/path/to/chromium overrides the browser.
import { $ } from "bun";
import { mkdir } from "node:fs/promises";
import { join } from "node:path";
import puppeteer, { type Page } from "puppeteer-core";

const root = join(import.meta.dir, "../..");
const shots = process.argv[2] ?? process.env.STROOPY_SCREENSHOT_DIR ?? "";
if (shots) await mkdir(shots, { recursive: true });

await $`bun run build.ts`.cwd(root).quiet();
const port = 4300 + Math.floor(Math.random() * 500);
const server = Bun.spawn(["bun", "src/dev.ts", "--dist"], { cwd: root, env: { ...process.env, PORT: String(port) }, stdout: "ignore" });
await Bun.sleep(500);

const browser = await puppeteer.launch({
  executablePath: process.env.CHROME ?? "/usr/bin/chromium-browser",
  args: ["--hide-scrollbars", "--lang=en-US"],
});
let failures = 0;

function check(cond: boolean, what: string) {
  if (!cond) {
    failures++;
    console.error("FAIL:", what);
  }
}

async function test(name: string, fn: (page: Page) => Promise<void>, page: Page) {
  try {
    await fn(page);
    console.log("PASS  ", name);
  } catch (e) {
    failures++;
    console.error("FAIL  ", name, e);
  }
}

// Game state as the app sees it (exposed on globalThis.stroopy for this test).
const state = (page: Page) =>
  page.evaluate(() => {
    const c = (globalThis as any).stroopy.controller;
    return { phase: c.phase.value, mode: c.mode.value, score: c.score.value, current: c.current.value };
  });

async function shot(page: Page, name: string) {
  await Bun.sleep(400); // let transitions and animations settle
  if (shots) await page.screenshot({ path: join(shots, `${name}.png`) });
}

async function resize(page: Page, width: number, height: number) {
  await page.setViewport({ width, height });
  await Bun.sleep(100);
}

async function key(page: Page, k: Parameters<Page["keyboard"]["press"]>[0], times = 1) {
  for (let i = 0; i < times; i++) await page.keyboard.press(k);
}

async function waitRunning(page: Page) {
  await page.waitForFunction(() => (globalThis as any).stroopy.controller.phase.value === "running", { timeout: 6000 });
}

const DIRECTION_KEYS = ["ArrowUp", "ArrowRight", "ArrowDown", "ArrowLeft"] as const;

const page = await browser.newPage();
await resize(page, 420, 820);
await page.goto(`http://localhost:${port}/`);
await page.waitForSelector(".home");

await test("home screens", async (p) => {
  await shot(p, "home-portrait");
  await resize(p, 1280, 720);
  await shot(p, "home-landscape");
}, page);

await test("classic by keyboard", async (p) => {
  await resize(p, 420, 820);
  await key(p, "Enter"); // first mode button has focus
  await waitRunning(p);
  check((await state(p)).mode === 1, "classic mode");
  await shot(p, "classic-portrait");
  for (let i = 0; i < 3; i++) {
    const { current } = await state(p);
    await key(p, current.ink === current.word ? "j" : "ArrowLeft");
    check((await state(p)).score === i + 1, `score ${i + 1}`);
  }
  await resize(p, 1280, 720);
  await shot(p, "classic-landscape");
  const { current } = await state(p);
  await key(p, current.ink === current.word ? "ArrowLeft" : "ArrowRight"); // wrong
  check((await state(p)).phase === "over", "game over");
  await p.waitForSelector(".result");
  await shot(p, "result-landscape");
  await resize(p, 420, 820);
  await shot(p, "result-portrait");
  await key(p, "n");
  check((await state(p)).phase === "countdown", "new game counts down");
  await key(p, "Escape");
  check((await state(p)).phase === "idle", "quit");
}, page);

await test("find by keyboard", async (p) => {
  await resize(p, 1280, 720);
  await p.waitForSelector(".home");
  await key(p, "ArrowDown", 2);
  await key(p, "Enter");
  await waitRunning(p);
  check((await state(p)).mode === 3, "find mode");
  await shot(p, "find-landscape");
  for (let i = 0; i < 3; i++) {
    const slot = (await state(p)).current.word;
    await key(p, DIRECTION_KEYS[slot ^ (slot >> 1)]!);
    check((await state(p)).score === i + 1, `score ${i + 1}`);
  }
  await key(p, String(1 + (await state(p)).current.word) as "1");
  check((await state(p)).score === 4, "digit key");
  await resize(p, 420, 820);
  await shot(p, "find-portrait");
  await key(p, "Escape");
}, page);

await test("tap by keyboard", async (p) => {
  await resize(p, 420, 820);
  await p.waitForSelector(".home");
  await key(p, "ArrowDown", 3);
  await key(p, "Enter");
  await waitRunning(p);
  check((await state(p)).mode === 4, "tap mode");
  await shot(p, "tap-portrait");
  for (let i = 0; i < 3; i++) {
    const { current } = await state(p);
    await key(p, DIRECTION_KEYS[current.trueCard]!);
    check((await state(p)).score === i + 1, `score ${i + 1}`);
  }
  await resize(p, 1280, 720);
  await shot(p, "tap-landscape");
  await key(p, "Escape");
}, page);

await test("help page", async (p) => {
  await resize(p, 420, 820);
  await p.waitForSelector(".home");
  // Focus chain on home: 4 modes, Multiplayer, Highscores, Enter code, Help, Settings.
  await key(p, "ArrowDown", 7);
  await key(p, "Enter");
  await p.waitForSelector(".help");
  await shot(p, "help-portrait");
  await resize(p, 1280, 720);
  await shot(p, "help-landscape");
  await key(p, "Escape");
  await p.waitForSelector(".home");
}, page);

await test("settings and backup round trip", async (p) => {
  await resize(p, 420, 820);
  await key(p, "ArrowDown", 8);
  await key(p, "Enter");
  await p.waitForSelector(".settings");
  await shot(p, "settings-portrait");
  await key(p, "Tab", 5);
  await key(p, "Enter");
  await p.waitForSelector(".backup");
  await shot(p, "backup-portrait");
  await resize(p, 1280, 720);
  await shot(p, "backup-landscape");
  const result = await p.evaluate(() => {
    const { highscores, backup } = (globalThis as any).stroopy;
    const before = highscores.highscores.value.length;
    const code: string = backup.exportCode();
    highscores.clear();
    return { before, code };
  });
  check(result.before > 0, "highscores exist (classic scored 3)");
  check(result.code.startsWith("SYB1-"), "backup code");
  const area = await p.$("textarea:not([readonly])");
  await area!.type("Here it is:\n" + result.code);
  await shot(p, "backup-import");
  await p.click("button.primary");
  const after = await p.evaluate(() => (globalThis as any).stroopy.highscores.highscores.value.length);
  check(after === result.before, `restored ${after} of ${result.before}`);
  const again = await p.evaluate(() => (globalThis as any).stroopy.backup.exportCode());
  check(again === result.code, "export after import is identical");
}, page);

await browser.close();
server.kill();
console.log(failures ? `${failures} failure(s)` : "all passed");
process.exit(failures ? 1 : 0);
