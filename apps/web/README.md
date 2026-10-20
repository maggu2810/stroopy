# Stroopy: web app (PWA)

An independent implementation of the Stroopy specs for browsers, installable
as a Progressive Web App. It is the way onto iOS without an App Store
account ([decision 0010](../../docs/decisions/0010-web-app-pwa.md)).

| | |
|---|---|
| stack | TypeScript, [Bun](https://bun.sh) (package manager, bundler, tests, dev server), Preact + signals |
| targets | any current browser; iOS/iPadOS via Safari *Add to Home Screen*; Android/desktop *Install app* |
| supports | rules **1**, share code **1**, backup **1** |
| licence | GPL-3.0-or-later; dependencies MIT / Apache-2.0 |

## Commands

```sh
bun install
bun run dev          # dev server with hot reload: http://localhost:3000
bun test             # conformance vectors (docs/spec/test-vectors) + translation check
bun run typecheck
bun run build        # production build into dist/ (static files, service worker)
bun run preview      # build and serve dist/ on http://localhost:3000
bun run test:ui      # browser UI test in headless Chromium; optional arg: screenshot dir
```

`HOST=0.0.0.0 bun run preview` makes it reachable from a phone on the same
network. Service workers (offline, install) need HTTPS or `localhost`.

## Deploying

`dist/` is plain static files with relative paths, so any static HTTPS host
works, also in a sub-directory (Codeberg Pages, GitHub Pages, own server).

## Layout

```
index.html            entry; build.ts adds manifest/icon links
build.ts              production build + service worker precache list
public/               manifest and icons (rendered by tools/render-assets.sh)
src/core/             spec implementations: rules, game, share code, backup
src/app/              state: settings, i18n, highscores, game controller, input
src/ui/               Preact pages (one per screen of docs/design/product.md)
src/sw.ts             service worker (offline cache)
tests/                bun test: conformance vectors, i18n; e2e/ui.ts browser test
```

Storage: `localStorage` with a request for persistent storage. Browsers may
still drop it (e.g. when the home-screen icon is removed), so players should
keep a backup code (Settings → Backup).
