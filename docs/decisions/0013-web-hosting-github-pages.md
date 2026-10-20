# 0013: Host the web app on GitHub Pages

Status: accepted (2026-10-20)

## Context
The web app (decision 0010) needs a static HTTPS host: iOS only installs a
PWA and runs its service worker over HTTPS. The repository is public on
GitHub (`maggu2810/stroopy`).

## Decision
- The web app is served from **GitHub Pages**:
  <https://maggu2810.github.io/stroopy/>.
- A GitHub Actions workflow (`.github/workflows/web-pages.yml`) builds it on
  every push to `main` that touches `apps/web/` or the test vectors. It runs
  the type check and the conformance tests first, so a failing build is never
  published. The Pages source is set to "GitHub Actions"; there is no
  `gh-pages` branch.
- The app uses relative paths only, so it works in the `/stroopy/`
  sub-directory and would work unchanged on an own domain.

## Consequences
- No server of our own, no cost. GitHub sees the IP addresses of visitors;
  the app itself makes no network calls except the Nostr relays the player
  opts into.
- Moving to another host (Codeberg Pages, own domain) only needs a new
  workflow. Installed PWAs are bound to their origin, so players would have
  to reinstall and restore a backup code.
