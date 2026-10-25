// SPDX-License-Identifier: GPL-3.0-or-later
// Live multiplayer test over the public Nostr relays (needs internet): two browser pages,
// one hosts a room, the other joins with the invitation link; both play the same game.
// Usage: bun run test:multiplayer [screenshot-dir]
import { $ } from "bun";
import { mkdir } from "node:fs/promises";
import { join } from "node:path";
import puppeteer, { type Page } from "puppeteer-core";

const root = join(import.meta.dir, "../..");
const shots = process.argv[2] ?? "";
if (shots) await mkdir(shots, { recursive: true });

await $`bun run build.ts`.cwd(root).quiet();
const port = 4300 + Math.floor(Math.random() * 500);
const server = Bun.spawn(["bun", "src/dev.ts", "--dist"], { cwd: root, env: { ...process.env, PORT: String(port) }, stdout: "ignore" });
await Bun.sleep(500);
const browser = await puppeteer.launch({ executablePath: process.env.CHROME ?? "/usr/bin/chromium-browser", args: ["--lang=en-US"] });

async function open(name: string, url: string): Promise<Page> {
  // Separate browser contexts: separate storage, like two devices.
  const context = await browser.createBrowserContext();
  const page = await context.newPage();
  page.on("console", (m) => console.log(`[${name}]`, m.text()));
  page.on("pageerror", (e) => console.log(`[${name}] error`, e));
  await page.setViewport({ width: 1000, height: 640 });
  await page.evaluateOnNewDocument((n) => localStorage.setItem("stroopy.playerName", JSON.stringify(n)), name);
  await page.goto(url);
  return page;
}

const game = (p: Page) =>
  p.evaluate(() => {
    const c = (globalThis as any).stroopy.controller;
    return { phase: c.phase.value as string, score: c.score.value as number, current: c.current.value };
  });

async function answerCorrectly(p: Page, times: number) {
  for (let i = 0; i < times; i++) {
    const { current } = await game(p);
    await p.keyboard.press(current.ink === current.word ? "j" : "f");
  }
}

let failed = false;
const step = (what: string) => console.log("…", what);
try {
  const base = `http://localhost:${port}/`;
  const ann = await open("Ann", base);
  await ann.waitForSelector(".home");
  await ann.click("button.multi");
  await ann.waitForSelector(".multiplayer");
  await ann.click("button.primary"); // Create room
  await ann.waitForSelector(".lobby");
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const code = await ann.evaluate(() => (globalThis as any).stroopy.multiplayer.room.value.invitation as string);
  step(`room ${code}`);

  const bob = await open("Bob", `${base}?j=${code}`);
  await bob.waitForSelector(".multiplayer");
  await Bun.sleep(300);
  await bob.keyboard.press("Enter"); // the prefilled join field has focus
  await bob.waitForSelector(".lobby");

  step("waiting until both see each other");
  await ann.waitForFunction(() => document.querySelectorAll(".players li").length === 2, { timeout: 20000 });
  await bob.waitForFunction(() => document.querySelectorAll(".players li").length === 2, { timeout: 20000 });
  if (shots) await ann.screenshot({ path: join(shots, "lobby-host.png") });
  if (shots) await bob.screenshot({ path: join(shots, "lobby-guest.png") });

  step("host starts classic");
  await ann.click(".lobby button.primary.tall");
  for (const p of [ann, bob]) await p.waitForFunction(() => (globalThis as any).stroopy.controller.phase.value === "running", { timeout: 15000 });
  const seeds = await Promise.all([ann, bob].map((p) => p.evaluate(() => (globalThis as any).stroopy.controller.seedHex)));
  if (seeds[0] !== seeds[1]) throw new Error(`different seeds ${seeds}`);

  await answerCorrectly(ann, 5);
  await answerCorrectly(bob, 2);
  step("waiting for progress to arrive");
  await bob.waitForFunction(() => document.querySelector(".opponent")?.textContent?.includes("Ann 5"), { timeout: 15000 });
  await ann.waitForFunction(() => document.querySelector(".opponent")?.textContent?.includes("Bob 2"), { timeout: 15000 });
  if (shots) await bob.screenshot({ path: join(shots, "game-guest.png") });

  step("Bob fails, Ann sees ✗");
  const { current } = await game(bob);
  await bob.keyboard.press(current.ink === current.word ? "f" : "j");
  await bob.waitForSelector(".ranking");
  await ann.waitForFunction(() => document.querySelector(".opponent")?.textContent?.includes("✗"), { timeout: 15000 });
  await ann.keyboard.press("Escape"); // quitting counts as out
  await ann.waitForSelector(".ranking");
  await bob.waitForFunction(() => document.querySelector(".ranking li")?.textContent?.includes("Ann") && document.querySelector(".ranking")?.textContent?.includes("out at #6"), { timeout: 15000 });
  if (shots) await ann.screenshot({ path: join(shots, "lobby-results-host.png") });
  if (shots) await bob.screenshot({ path: join(shots, "lobby-results-guest.png") });
  console.log("PASS   multiplayer over Nostr");
} catch (e) {
  failed = true;
  for (const p of (await browser.pages())) {
    console.error(await p.evaluate(() => {
      const r = (globalThis as any).stroopy?.multiplayer.room.value;
      return r && JSON.stringify({ connected: r.connected.value, me: r.me.value, players: r.players.value, host: r.hostKey.value });
    }).catch(String));
  }
  console.error("FAIL  ", e);
}
await browser.close();
server.kill();
process.exit(failed ? 1 : 0);
