# 0010: A web app (PWA) for iOS and everyone without an install

Status: accepted (2026-10-20)

## Context
iOS is wanted, but there will be no paid Apple developer account. Without
one, every native route fails for the people we share with:

| route | why not |
|---|---|
| free Apple ID signing (Xcode) | each user needs a Mac; re-sign every 7 days; max. 3 apps |
| AltStore / SideStore | still 7-day re-signing, work on the user's side |
| EU alternative marketplaces | still need Apple notarisation, i.e. a paid account |
| TestFlight, App Clips | paid account |
| Swift Playgrounds package | iPad and Mac only, not iPhone |

Safari's *Add to Home Screen* installs a web app with its own icon,
fullscreen and offline support, with no account and no review.

## Decision
- New implementation **`apps/web/`**: a **Progressive Web App** in
  **TypeScript**, built and tested with **Bun** (package manager, bundler,
  test runner, dev server). UI with **Preact** + signals (MIT, ~5 kB);
  Nostr signatures with **@noble/curves** / **@noble/hashes** (MIT, audited,
  no dependencies).
- It is an independent implementation of `docs/spec/` (decision 0005) and
  passes the same conformance vectors (`bun test`). It does not reuse the Qt
  code, and Qt for WebAssembly is not used: it would be a multi-MB download
  with a slow start, and iOS Safari is its weakest target.
- It follows `docs/design/product.md` like every other app, including the
  responsive portrait/landscape layouts and keyboard/gamepad input.
- Hosting: any static HTTPS host (e.g. Codeberg Pages, GitHub Pages, own
  domain). This is not a game server: nothing runs there but file delivery.

## Consequences
- Works on iOS, Android, desktop and TVs with a browser, without install.
- Browser limits: no Bluetooth (Safari has no Web Bluetooth), no raw LAN
  sockets. Live multiplayer in the browser is **Nostr only** (WebSockets).
  This moves Nostr ahead of LAN in the transport order (decision 0012).
- Browser storage can be lost (home-screen icon removed, storage pressure,
  new phone). The app asks for persistent storage and offers an export/import
  backup code (decision 0011).
