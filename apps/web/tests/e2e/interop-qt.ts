// SPDX-License-Identifier: GPL-3.0-or-later
// Live interop over the public Nostr relays (needs internet): the Qt app's multiplayer stack
// (apps/qt/build/linux/tests/interop_nostr) plays against this web app in a browser.
// Scenario 1: Qt hosts, web joins. Scenario 2: web hosts, Qt joins.
// Both sides answer correctly and then fail; each must see the other's final result.
// Usage: bun run test:interop   (build apps/qt with the linux preset first)
import { $ } from "bun";
import { join } from "node:path";
import puppeteer, { type Page } from "puppeteer-core";

const root = join(import.meta.dir, "../..");
const qtDriver = join(root, "../qt/build/linux/tests/interop_nostr");
if (!(await Bun.file(qtDriver).exists())) throw new Error(`build the Qt app first: ${qtDriver}`);

await $`bun run build.ts`.cwd(root).quiet();
const port = 4300 + Math.floor(Math.random() * 500);
const server = Bun.spawn(["bun", "src/dev.ts", "--dist"], { cwd: root, env: { ...process.env, PORT: String(port) }, stdout: "ignore" });
await Bun.sleep(500);
const browser = await puppeteer.launch({ executablePath: process.env.CHROME ?? "/usr/bin/chromium-browser", args: ["--lang=en-US"] });

/** Starts the Qt driver; resolves lines as they arrive. */
function startQt(args: string[]) {
  const proc = Bun.spawn([qtDriver, ...args], { env: { ...process.env, QT_QPA_PLATFORM: "offscreen" }, stdout: "pipe", stderr: "inherit" });
  const lines: string[] = [];
  const waiters: { prefix: string; resolve: (l: string) => void }[] = [];
  (async () => {
    let buffer = "";
    for await (const chunk of proc.stdout) {
      buffer += new TextDecoder().decode(chunk);
      let i;
      while ((i = buffer.indexOf("\n")) >= 0) {
        const line = buffer.slice(0, i);
        buffer = buffer.slice(i + 1);
        console.log("  [qt]", line);
        lines.push(line);
        for (const w of waiters.filter((w) => line.startsWith(w.prefix))) w.resolve(line);
      }
    }
  })();
  const next = (prefix: string, timeoutMs = 60000) =>
    new Promise<string>((resolve, reject) => {
      const hit = lines.find((l) => l.startsWith(prefix));
      if (hit) return resolve(hit);
      waiters.push({ prefix, resolve });
      setTimeout(() => reject(new Error(`Qt: no "${prefix}" within ${timeoutMs} ms`)), timeoutMs);
    });
  return { proc, lines, next };
}

async function openWeb(url: string): Promise<Page> {
  const context = await browser.createBrowserContext();
  const page = await context.newPage();
  await page.setViewport({ width: 1000, height: 640 });
  await page.evaluateOnNewDocument(() => localStorage.setItem("stroopy.playerName", JSON.stringify("WebBot")));
  await page.goto(url);
  return page;
}

/** In the running game: answer `right` items correctly, then one wrongly. */
async function playWeb(page: Page, right: number) {
  await page.waitForFunction(() => (globalThis as any).stroopy.controller.phase.value === "running", { timeout: 30000 });
  for (let i = 0; i <= right; i++) {
    const c = await page.evaluate(() => (globalThis as any).stroopy.controller.current.value);
    const correct = c.ink === c.word ? "j" : "f";
    await page.keyboard.press(i < right ? correct : correct === "j" ? "f" : "j");
    await Bun.sleep(250);
  }
}

const webSeed = (page: Page) => page.evaluate(() => (globalThis as any).stroopy.controller.seedHex as string);
const waitRanking = (page: Page, text: string) =>
  page.waitForFunction((t) => document.querySelector(".ranking")?.textContent?.includes(t), { timeout: 60000 }, text);

let failures = 0;
async function scenario(name: string, fn: () => Promise<void>) {
  console.log(name);
  try {
    await fn();
    console.log("PASS  ", name);
  } catch (e) {
    failures++;
    console.error("FAIL  ", name, e);
  }
}

await scenario("Qt hosts, web joins", async () => {
  const qt = startQt(["host"]);
  try {
    const code = (await qt.next("ROOM")).split(" ")[1]!;
    const web = await openWeb(`http://localhost:${port}/?j=${code}`);
    await web.waitForSelector(".multiplayer");
    await Bun.sleep(300);
    await web.keyboard.press("Enter");
    await web.waitForFunction(() => document.querySelector(".players")?.textContent?.includes("QtBot"), { timeout: 30000 });
    const play = await qt.next("PLAY");
    await playWeb(web, 2);
    if (play.split(" ")[2] !== (await webSeed(web))) throw new Error("different seeds");
    await waitRanking(web, "QtBot");
    await waitRanking(web, "out at #5");
    const results = await Promise.all([qt.next("RESULT QtBot"), qt.next("RESULT WebBot")]);
    if (results[0] !== "RESULT QtBot 4 5" || results[1] !== "RESULT WebBot 2 3") throw new Error(`Qt saw ${results}`);
    if ((await qt.proc.exited) !== 0) throw new Error("Qt driver failed");
    await web.close();
  } finally {
    qt.proc.kill();
  }
});

await scenario("web hosts, Qt joins", async () => {
  const web = await openWeb(`http://localhost:${port}/`);
  await web.waitForSelector(".home");
  await web.click("button.multi");
  await web.waitForSelector(".multiplayer");
  await web.click("button.primary"); // Create room
  await web.waitForSelector(".lobby");
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const code = await web.evaluate(() => (globalThis as any).stroopy.multiplayer.room.value.invitation as string);
  const qt = startQt(["join", code]);
  try {
    await web.waitForFunction(() => document.querySelector(".players")?.textContent?.includes("QtBot"), { timeout: 30000 });
    await web.click(".lobby button.primary.tall"); // Start (classic is preselected)
    const play = await qt.next("PLAY");
    await playWeb(web, 3);
    if (play.split(" ")[2] !== (await webSeed(web))) throw new Error("different seeds");
    await waitRanking(web, "out at #5");
    const results = await Promise.all([qt.next("RESULT QtBot"), qt.next("RESULT WebBot")]);
    if (results[0] !== "RESULT QtBot 4 5" || results[1] !== "RESULT WebBot 3 4") throw new Error(`Qt saw ${results}`);
    if ((await qt.proc.exited) !== 0) throw new Error("Qt driver failed");
  } finally {
    qt.proc.kill();
  }
});

await browser.close();
server.kill();
console.log(failures ? `${failures} failure(s)` : "all passed");
process.exit(failures ? 1 : 0);
