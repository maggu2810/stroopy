# Proposal: building the Qt Android app with GitHub Actions

Status: proposal, not decided. If adopted, record the choice in
`docs/decisions/` (option A or B, where APKs are published, signing).

Today GitHub Actions only builds and publishes the web app
(`.github/workflows/web-pages.yml`). The Qt app (`apps/qt/`) is built locally:
`tools/setup-qt.sh` installs Qt 6.12.0 into `~/Qt-aqt` via aqtinstall, and the
CMake presets expect the Android SDK in `~/Android/Sdk` with NDK
`27.2.12479018` and platform `android-37.0`.

The same build can run on GitHub Actions. Installing Qt there is not costly:
aqtinstall downloads prebuilt archives, nothing is compiled. A cache is enough;
a custom container image is optional.

## Option A (recommended): standard runner + cache

The `ubuntu-latest` runner already has Java, the Android SDK, several NDKs,
Ninja and CMake installed. What is missing:

| Need | How |
|---|---|
| Qt 6.12.0, desktop kit (host) + `android_arm64_v8a`, with our modules | `jurplel/install-qt-action` (wraps aqtinstall) with `cache: true`, once per kit. First run downloads; later runs restore from the Actions cache in ~20–40 s. Cache key = Qt version + modules, so bumping the version refreshes it automatically. |
| NDK `27.2.12479018`, platform `android-37.0` | `sdkmanager "ndk;27.2.12479018" "platforms;android-37.0"` if not preinstalled; takes seconds. |
| CMake ≥ 4 | Check the runner's version; otherwise `lukka/get-cmake`. |
| libsecp256k1 and KDAB OpenSSL (FetchContent) | Optionally cache the FetchContent download directory with `actions/cache`. |

The presets hard-code `$env{HOME}/Qt-aqt/...` and `~/Android/Sdk`. CI needs
either a `ci-android` preset that reads paths from environment variables
(`QT_ROOT_DIR`, `ANDROID_SDK_ROOT`, `ANDROID_NDK_ROOT`), or must install Qt into
`~/Qt-aqt` so the existing presets work unchanged.

## Option B: prebuilt container image

1. A Containerfile in the repo (e.g. `.github/ci/qt-android.Containerfile`)
   based on `fedora:45`: CMake, Ninja, a JDK, Android cmdline-tools, NDK and
   platform, and aqt for both Qt kits.
2. A separate workflow builds it and pushes it to GHCR (e.g.
   `ghcr.io/maggu2810/stroopy-qt-android:6.12.0`). It runs only when the
   Containerfile changes or on manual trigger.
3. The APK workflow uses `jobs.<id>.container: ghcr.io/...:6.12.0`. Public
   GHCR packages are free to pull.

Pros:

- Exactly reproducible environment; the same image runs locally with
  `podman run`.
- Fedora instead of Ubuntu, matching local development.

Cons:

- The image is several GB (the NDK alone is ~2 GB unpacked); pulling it on
  every run likely takes about as long as restoring the cache in option A.
- More maintenance: rebuild on every Qt or NDK bump and for Fedora updates.

The container buys reproducibility, not speed. Start with A; switch to B if CI
and local builds should be identical.

## Applies to both options

- **Signing key.** A CI runner generates a fresh debug keystore each time, so
  every build would have a different signature and users could not update one
  build over another. Store one fixed keystore as a base64 GitHub secret and
  pass it to androiddeployqt (`QT_ANDROID_SIGN_APK`,
  `QT_ANDROID_KEYSTORE_PATH`, `QT_ANDROID_KEYSTORE_ALIAS` and the store/key
  password variables), even for debug-style builds.
- **Distribution.** Workflow artifacts expire (90 days by default) and need a
  GitHub login to download. Since builds are shared directly (decision 0008),
  attach the APK to a GitHub Release on tag pushes (e.g. `v*`); push builds can
  still upload artifacts for testing.
- **Linux job.** The same workflow can run `cmake --preset linux` and `ctest`
  with `QT_QPA_PLATFORM=offscreen`, giving the Qt app CI coverage of the
  conformance vectors, like `bun test` does for the web app.
