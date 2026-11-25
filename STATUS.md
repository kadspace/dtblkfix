# STATUS

**What it is**: DtBlkFix, a fork of Darrell Tam's DtBlkFx (dozius fork) that fixes what makes it hard
to use today and leaves the engine and sound alone. VST3 for 64-bit Windows and Apple Silicon Macs,
GPLv3.

## Current state
- 1.0.0 (Windows) is merged to `main` and released (tag `v1.0.0`). The release is one file,
  `DtBlkFix.vst3`: the resources and licenses are packed into it (`tools/pack_resources.py`) and
  unpacked to `%LOCALAPPDATA%\DtBlkFix\<version>` on first load (`plugin/EngineBridge.cpp`).
- 1.1.0 adds macOS (PR #3, merged; not released yet). The engine
  and VSTGUI 3.5 editor are Windows code; on the Mac they run on `softwin/`, a software stand-in for
  the Windows API calls they use (GDI drawing, windows, messages, menus, an edit box, files), and
  `plugin/SoftwinView.cpp` shows the editor in JUCE at the display's pixel density. Arm64 VST3 +
  standalone, FFTW 3.3.10 linked statically.
- Windows is unchanged by the Mac work: with MSVC `/Brepro`, 53 of the 54 engine objects are
  byte-identical to `main`'s and the last has the same machine code.
- Built with CMake + JUCE 8 (AGPLv3, kept on purpose; the VST3 SDK was the MIT alternative). No VST2
  SDK needed: `shim/` stands in for it.
- CI (`build.yml`): the Windows job builds, runs ctest (43/43 factory presets bit-identical to the
  original without a host transport), drags the resize grip and screenshots every size, skin and
  style. The macOS job (macos-15, arm64) builds FFTW from source, runs ctest (state, editor clicks
  and preset saving, sync), renders the presets, screenshots the editor, signs ad hoc, runs
  pluginval at strictness 5 and uploads `DtBlkFix-macOS.zip`. The compare job measures the Mac
  renders against Windows: 32/43 presets within -80 dB, the pitch-tracking ones drift apart after a
  while (same levels), as they also do between two Linux builds that differ only in FMA.

## Next up
1. Tag `v1.1.0` (the release attaches both platforms).
2. Try the VST3 in a DAW on an Apple Silicon Mac: install per the README, check the editor, menus,
   the preset name box, the value popups while dragging, and resizing.
3. Launch: Gumroad listing (pay what you want, suggested $0), KVR, Reddit, producer Discords. The
   launch copy is kept outside this repo.
4. Known issues to fix in a later release: not code-signed or notarized (Mac users need the `xattr`
   line in the README); opens at 100% on high-DPI Windows screens until resized.
5. Engine bugs found and left alone for 1.0 (see PR #1, "Not done"): WarpMix reads past the spectrum,
   Smear shares one random generator, 32-bit sample counters.

## Gotchas
- Releases: push a `v<version>` tag; `release.yml` makes a draft release with `DtBlkFix.vst3` and
  `DtBlkFix-macOS.zip`, which is published by hand.
- A user's own skins live in the unpacked folder, which is per version: a new version starts without
  them. On the Mac they're inside the signed bundle, so adding one there breaks the signature.
- The engine assumes `long` is 32 bits (pixel rows, the state chunk, prbs32). macOS has 64-bit
  `long`: those spots use `ulong32`/`long32`, which are `unsigned long`/`long` on Windows.
- Shared code must stay identical on Windows: put Mac changes behind `_WIN32`/`DTBLKFIX_SOFTWIN`.
  Off Windows the engine is built with `WIN32` (only `vstgui.h` reads it, to pick its Windows code)
  but never `_WIN32`.
- Develop the Mac side on Linux (or WSL): clang, `libfftw3-dev`, JUCE's X11 deps, and Xvfb +
  xdotool for the standalone app. `tools/softwin_snapshots.sh` renders the editor headless.
- `softwin/windows.h` turns `fopen` into a macro that converts `\` paths; it must include `<cstdio>`
  first, and softwin's own sources `#undef` it.
- Xcode 16's libc++ has no `std::auto_ptr`; `misc_stuff.h` supplies one there.
- pedalboard 0.9.26 (Python) failed to load any VST3 on Windows here, other vendors' included; use
  `tests/host_test.cpp` (JUCE's host) instead.
- `projects/DTBlkFx.sln` still builds the VST2 for anyone with the SDK; CI doesn't build it. The
  MinGW build of the engine is broken on `main` (two-phase template lookup); MSVC and clang work.
- Removing an effect row (×) moves the effects below up a slot, so automation recorded on those slots
  then drives a different effect.
- Read the "Not done" list in PR #1 before changing the engine.

**Last updated**: 2026-11-24
