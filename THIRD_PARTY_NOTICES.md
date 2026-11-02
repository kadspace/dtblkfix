# Third-party notices

| Component | Where | Copyright | License |
|---|---|---|---|
| DtBlkFx | `dtblkfx/`, `docs/`, `resources/`, `support/` | © 2005–2006 Darrell Tam | GPLv2 or later |
| VSTGUI 3 | `vstgui/` | © 2004 Steinberg Media Technologies | BSD 3-clause (in each file's header) |
| libpng 1.2.10 | `libpng/` | © 1995–2006 Guy Eric Schalnat, Andreas Dilger, Glenn Randers-Pehrson | libpng license, [`libpng/LICENSE`](libpng/LICENSE) |
| zlib 1.2.3 | `zlib/` | © 1995–2005 Jean-loup Gailly and Mark Adler | zlib license, [`zlib/README`](zlib/README) |
| FFTW 3.3.5 headers | `fftw/` | © 2003, 2007–2014 Matteo Frigo and Massachusetts Institute of Technology | GPLv2 or later, [`fftw/COPYING`](fftw/COPYING) |

FFTW itself isn't in this repo. The plugin loads `libfftw3f-3.dll` at runtime, and any binary release ships that DLL under the GPL.

The Steinberg VST2 SDK is required to build but isn't included. VST is a trademark of Steinberg Media Technologies GmbH.

DtBlkFix as a whole is distributed under GPLv3 ([`LICENSE`](LICENSE)), which DtBlkFx's "or later" clause allows.
