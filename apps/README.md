# apps/

One directory per platform implementation, each built natively from the
normative specs in [`docs/spec/`](../docs/spec/):

| dir | platform | stack | status |
|---|---|---|---|
| `android/` | Android | Kotlin + Jetpack Compose | planned |
| `ios/` | iOS | Swift + SwiftUI | future |
| `desktop/` | Linux, Windows, macOS | to be decided (see decision 0005) | future |

Every implementation must pass the shared conformance vectors in
`docs/spec/test-vectors/`.
