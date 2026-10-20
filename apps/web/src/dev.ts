// SPDX-License-Identifier: GPL-3.0-or-later
// Development server with hot reload (bun run dev), or `--dist` to serve the production build.
import { join } from "node:path";
import index from "../index.html";

const port = Number(process.env.PORT ?? 3000);
const dist = process.argv.includes("--dist") ? join(import.meta.dir, "../dist") : null;
const pub = join(import.meta.dir, "../public");

async function serveFile(req: Request, base: string): Promise<Response> {
  const path = decodeURIComponent(new URL(req.url).pathname);
  const file = Bun.file(join(base, path === "/" ? "index.html" : path));
  return !path.includes("..") && (await file.exists()) ? new Response(file) : new Response("not found", { status: 404 });
}

const hostname = process.env.HOST ?? "localhost";
const server = dist
  ? Bun.serve({ port, hostname, fetch: (req) => serveFile(req, dist) })
  : Bun.serve({
      port,
      hostname,
      development: { hmr: true, console: true },
      routes: { "/": index },
      fetch: (req) => serveFile(req, pub),
    });
console.log(`Stroopy ${dist ? "(dist)" : "(dev)"}: ${server.url}`);
