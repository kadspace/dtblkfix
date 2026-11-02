# DtBlkFix

DtBlkFx, fixed.

DtBlkFx is Darrell Tam's FFT effect from 2005–2006: up to eight spectral effects in series, from harmonic filtering and pitch tracking to vocoding and contrast. There's still nothing quite like it, but a few things make it hard to use today. DtBlkFix fixes those and leaves the rest alone. It runs the same engine and sounds the same, and the original presets still work.

## Fixed

- **Timing.** DtBlkFx delayed its output by one block but told the host its latency was zero, so at the default settings everything came out a beat late. DtBlkFix reports the delay, and the host compensates for it. Apart from that, the audio is bit-identical to the original.

## Planned

- **Song sync.** Blocks will follow the host's song position on every buffer, so they stay on the beat through loops, jumps and tempo changes.
- **Resizable window.** The original is a fixed 2006-sized window, which is tiny on a modern screen.
- **Skins.** The original look plus a dark one. A skin is just the folder of PNGs the plugin already loads.
- **VST3 and a Windows installer**, built by CI.

That's the whole list. There are no new effects or controls.

## Building

Windows only for now. You need Visual Studio 2022 and the Steinberg VST2 SDK, which isn't included because it isn't redistributable. Copy its `pluginterfaces/` and `public.sdk/` folders into `vstsdk/`, then open `projects/DTBlkFx.sln` or run `build_fork.bat`.

At runtime the plugin loads `libfftw3f-3.dll` ([FFTW 3](https://www.fftw.org/), single precision). It loads its images and `presets.txt` from a `dtblkfx` folder next to the DLL; copy `resources/stereo_*` there.

DtBlkFix has its own plugin ID, so it installs next to the original DtBlkFx and doesn't replace it.

## Credits

DtBlkFx was written by Darrell Tam. This repo starts from the [dozius fork](https://github.com/dozius/dtblkfx), which got the source building for x64 and has since moved off GitHub. DtBlkFix isn't affiliated with or endorsed by either of them. The original manual is in [`docs/manual.html`](docs/manual.html), and the fork's readme is in [`docs/dtblkfx-readme.md`](docs/dtblkfx-readme.md).

## License

GPLv3, see [`LICENSE`](LICENSE). Bundled third-party code and its licenses are listed in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).
