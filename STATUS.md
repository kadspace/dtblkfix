# STATUS

**What it is**: DtBlkFix, a fork of Darrell Tam's DtBlkFx (dozius fork) that fixes what makes it hard
to use today and leaves the engine and sound alone. 64-bit VST3 for Windows, GPLv3.

## Current state
- 1.0.0 is merged to `main` and released (tag `v1.0.0`). The release is one file, `DtBlkFix.vst3`: the
  resources and licenses are packed into it (`tools/pack_resources.py`) and unpacked to
  `%LOCALAPPDATA%\DtBlkFix\<version>` on first load (`plugin/EngineBridge.cpp`).
- Built with CMake + JUCE 8 (AGPLv3, kept on purpose; the VST3 SDK was the MIT alternative). No VST2
  SDK needed: `shim/` stands in for it.
- CI (`build.yml`) builds, runs ctest (43/43 factory presets bit-identical without a host transport),
  drags the resize grip and screenshots every size, skin and style. `tests/host_test.cpp` loads the
  single file in JUCE's VST3 host from an empty AppData (unpack, then reuse).

## Next up
1. Launch: Gumroad listing (pay what you want, suggested $0), KVR, Reddit, producer Discords. The
   launch copy is kept outside this repo.
2. Known issues to fix in a later release: not code-signed; opens at 100% on high-DPI screens until
   resized.
3. Engine bugs found and left alone for 1.0 (see PR #1, "Not done"): WarpMix reads past the spectrum,
   Smear shares one random generator, 32-bit sample counters.

## Gotchas
- Releases: push a `v<version>` tag; `release.yml` makes a draft release with `DtBlkFix.vst3`, which
  is published by hand.
- A user's own skins live in the unpacked folder, which is per version: a new version starts without
  them.
- pedalboard 0.9.26 (Python) failed to load any VST3 on Windows here, other vendors' included; use
  `tests/host_test.cpp` (JUCE's host) instead.
- `projects/DTBlkFx.sln` still builds the VST2 for anyone with the SDK; CI doesn't build it.
- Removing an effect row (×) moves the effects below up a slot, so automation recorded on those slots
  then drives a different effect.
- Read the "Not done" list in PR #1 before changing the engine.

**Last updated**: 2026-11-11
