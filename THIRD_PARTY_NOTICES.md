# Third-party notices

## In this repo

| Component | Where | Copyright | License |
|---|---|---|---|
| DtBlkFx | `dtblkfx/`, `docs/`, `resources/`, `support/` | © 2005–2006 Darrell Tam | GPLv2 or later |
| VSTGUI 3 | `vstgui/` | © 2004 Steinberg Media Technologies | BSD 3-clause (in each file's header) |
| libpng 1.2.10 | `libpng/` | © 1995–2006 Guy Eric Schalnat, Andreas Dilger, Glenn Randers-Pehrson | libpng license, [`libpng/LICENSE`](libpng/LICENSE) |
| zlib 1.2.3 | `zlib/` | © 1995–2005 Jean-loup Gailly and Mark Adler | zlib license, [`zlib/README`](zlib/README) |
| FFTW 3.3.5 headers | `fftw/` | © 2003, 2007–2014 Matteo Frigo and Massachusetts Institute of Technology | GPLv2 or later, [`fftw/COPYING`](fftw/COPYING) |

`shim/`, `plugin/`, `tests/`, `tools/`, `cmake/` and `packaging/` are DtBlkFix's own, under GPLv3. `shim/` is a clean-room stand-in for the parts of the VST 2 plugin base classes that the engine uses, written from the engine's code, not from Steinberg's headers. The Dark and Warm skins are made at build time from the DtBlkFx images in `resources/`.

## In the binaries, not in this repo

| Component | Version | Copyright | License |
|---|---|---|---|
| JUCE | 8.0.15 | © Raw Material Software Limited | AGPLv3 (or the commercial JUCE licence) |
| VST3 SDK, as bundled with JUCE | | © 2025 Steinberg Media Technologies | MIT |
| FFTW, `libfftw3f-3.dll` | 3.3.5, 64-bit, the official Windows build | © 2003, 2007–2014 Matteo Frigo and Massachusetts Institute of Technology | GPLv2 or later |

CI clones JUCE at the tag above and downloads the FFTW DLLs from [fftw.org](https://www.fftw.org/install/windows.html), checking the zip's SHA-256. The release zip and the installer include the license texts of FFTW, JUCE and the VST3 SDK in `licenses/`. FFTW's source is at [fftw.org](https://www.fftw.org/download.html); JUCE's is at [github.com/juce-framework/JUCE](https://github.com/juce-framework/JUCE).

JUCE's other bundled dependencies that end up in the binaries (zlib, pnglib, jpeglib, FLAC, Ogg Vorbis, HarfBuzz, SheenBidi and others) are listed with their licenses in JUCE's `LICENSE.md`.

## Not included

The Steinberg VST2 SDK is only needed to build the VST2 version with `projects/DTBlkFx.sln`, and isn't included. VST is a trademark of Steinberg Media Technologies GmbH.

DtBlkFix as a whole is distributed under GPLv3 ([`LICENSE`](LICENSE)), which DtBlkFx's "or later" clause allows. The plugin binaries combine it with JUCE under the AGPLv3, which GPLv3 section 13 permits; the JUCE parts stay under the AGPLv3.
