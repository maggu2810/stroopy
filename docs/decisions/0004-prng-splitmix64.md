# 0004: PRNG: SplitMix64

Status: accepted (2026-10-19)

## Context
Shared seeds and multiplayer require bit-identical sequences on every
platform and in every language.

## Decision
Use **SplitMix64** (64-bit state, wrapping unsigned arithmetic), with an
exactly specified derivation of bounded integers and booleans, defined in
`docs/spec/rules/v1.md`. No platform RNGs and no floating point in
generation.

## Consequences
SplitMix64 is a few lines in any language and has no licence issues.
Implementations need unsigned 64-bit wrap-around: `uint64_t` in C++,
`BigInt` masked to 64 bits in TypeScript, masking in the Python reference
(`ULong` in Kotlin, `&+`/`&*` in Swift). Test vectors pin the raw output and
derived items.
