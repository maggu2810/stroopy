# 0014: Bump the app version on every change

Status: accepted (2026-10-25)

## Context
Builds are shared directly, without a store (decision 0008), and the web
app is published on every push (decision 0013). Until now the app versions
stayed at `0.1.0` while the apps changed, so a bug report, a screenshot of
the Help page or an installed APK did not tell which state of the code it
came from. Android also refuses to update an installed app to an APK with
the same or a lower `versionCode`.

App versions remain informational only and never decide compatibility
(decision 0007).

## Decision
- Every app in `apps/` has its own version in
  [SemVer](https://semver.org/) form `MAJOR.MINOR.PATCH`, independent of the
  other apps:
  - `apps/qt/`: `project(… VERSION …)` in `apps/qt/CMakeLists.txt`;
  - `apps/web/`: `"version"` in `apps/web/package.json`.
- **Every commit that changes what an app builds or ships bumps that app's
  version in the same commit**: source, QML/TSX, resources, translations,
  manifests and build settings that affect the result. Commits that only
  touch tests, READMEs, CI or `docs/` don't bump.
- Which part to bump:
  - `PATCH` for fixes and changes the player doesn't notice;
  - `MINOR` for new features or changed behaviour;
  - `MAJOR` stays `0` until the first release declared stable; after that
    it marks big, deliberate steps (e.g. dropping a supported
    compatibility version).
- One bump per commit, even if a series of commits lands together.
- The Android `versionCode` is derived from the version, so it increases
  with it: `MAJOR × 1 000 000 + MINOR × 1 000 + PATCH` (each part below
  1000).

## Consequences
- The version shown in the Help page identifies the build.
- Android installs a newer APK over an older one without uninstalling.
- Changes to several apps in one commit bump each of them.
