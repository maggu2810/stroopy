// SPDX-License-Identifier: GPL-3.0-or-later
// Production build into dist/: bundles index.html, copies public/, writes the service worker.
import { cp, readdir, rm } from "node:fs/promises";
import { join, relative } from "node:path";

const root = import.meta.dir;
const dist = join(root, "dist");
await rm(dist, { recursive: true, force: true });

const app = await Bun.build({
  entrypoints: [join(root, "index.html")],
  outdir: dist,
  minify: true,
  sourcemap: "linked",
  publicPath: "./",
});
if (!app.success) throw new AggregateError(app.logs, "build failed");

await cp(join(root, "public"), dist, { recursive: true });

const indexFile = Bun.file(join(dist, "index.html"));
const links = [
  '<link rel="manifest" href="./manifest.webmanifest">',
  '<link rel="icon" href="./icons/icon.svg" type="image/svg+xml">',
  '<link rel="apple-touch-icon" href="./icons/apple-touch-icon.png">',
].join("");
await Bun.write(indexFile, (await indexFile.text()).replace("</head>", links + "</head>"));

async function files(dir: string): Promise<string[]> {
  const out: string[] = [];
  for (const entry of await readdir(dir, { withFileTypes: true })) {
    const path = join(dir, entry.name);
    if (entry.isDirectory()) out.push(...(await files(path)));
    else if (!entry.name.endsWith(".map")) out.push(path);
  }
  return out;
}
const precache = ["./", ...(await files(dist)).map((f) => "./" + relative(dist, f))];
const hasher = new Bun.CryptoHasher("sha256");
for (const f of precache.slice(1)) hasher.update(await Bun.file(join(dist, f)).bytes());

const sw = await Bun.build({
  entrypoints: [join(root, "src/sw.ts")],
  outdir: dist,
  minify: true,
  naming: "sw.js",
  define: {
    PRECACHE: JSON.stringify(precache),
    CACHE_NAME: JSON.stringify("stroopy-" + hasher.digest("hex").slice(0, 12)),
  },
});
if (!sw.success) throw new AggregateError(sw.logs, "service worker build failed");

console.log(`dist/: ${precache.length - 1} files precached`);
