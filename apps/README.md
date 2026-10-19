# apps/

One directory per **implementation**: a toolkit/language combination. An
implementation may target one or several platforms, and a platform may have
several implementations (e.g. Android via Compose *and* via Qt).

Naming: `<stack>[-<platform>]`. Add the platform suffix when the stack is
used for one platform only (e.g. `compose-android`, `swiftui-ios`). Omit it
when the implementation is cross-platform (e.g. `qt`).

| dir | stack | targets | status |
|---|---|---|---|
| `qt/` | C++23 + Qt 6 / QML | Linux, Android (later Windows, macOS, iOS) | **active** |
| `compose-android/` | Kotlin + Jetpack Compose | Android | postponed |
| `swiftui-ios/` | Swift + SwiftUI | iOS | future |

Every implementation:
- has a `README.md` stating its stack, its targets, its application ids and
  the **compatibility versions it supports** (see
  [decision 0007](../docs/decisions/0007-compatibility-versions.md));
- passes the conformance vectors in `docs/spec/test-vectors/` for every
  version it claims to support.

Application ids: each implementation uses its own id per platform, so two
implementations can be installed side by side (e.g. the Compose app as
`de.maggu2810.android.stroopy`, a Qt Android build under a different id).
