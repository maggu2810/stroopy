# third-party/

Material from other apps, kept **for study only**. One sub-directory per
Android package id:

```
third-party/<package-id>/
  apk/          downloaded APK(s)            (git-ignored)
  decompiled/   jadx + apktool output        (git-ignored)
  NOTES.md      provenance (source URL, version, sha256)
```

Nothing here may be copied into `app/`: no code, layouts, images, sounds,
fonts or strings. Findings go into `docs/research/` in our own words.

To reproduce: `tools/fetch-original.sh`.
