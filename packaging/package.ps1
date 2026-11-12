# Put the built plugin together for release: the single-file VST3, in <out>.
#
#   package.ps1 -Build <cmake build folder> -Out <folder>
#
# <out>/DtBlkFix.vst3    the plugin, one file, to copy into the VST3 folder as it is. It carries the
#                        "dtblkfx" folder (FFTW, the presets, the images and the skins) and the
#                        licenses (DtBlkFix's GPLv3, THIRD_PARTY_NOTICES.md, and the FFTW, JUCE and
#                        VST3 SDK licenses), and unpacks them to %LOCALAPPDATA%\DtBlkFix\<version>
#                        the first time it loads.

param(
  [Parameter(Mandatory = $true)][string]$Build,
  [Parameter(Mandatory = $true)][string]$Out
)
$ErrorActionPreference = "Stop"

$plugin = Join-Path $Build "plugin/DtBlkFix_artefacts/Release/VST3/DtBlkFix.vst3/Contents/x86_64-win/DtBlkFix.vst3"
$pack = Join-Path $Build "plugin/dtblkfix_resources.pak"
$dst = Join-Path $Out "DtBlkFix.vst3"

if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }
New-Item -ItemType Directory -Force $Out | Out-Null
Copy-Item $plugin $dst

# everything the plugin needs must be in the pack, and the pack in the plugin
$names = [System.Text.Encoding]::UTF8.GetString([System.IO.File]::ReadAllBytes($pack))
foreach ($f in "dtblkfx\libfftw3f-3.dll", "dtblkfx\stereo_presets.txt", "dtblkfx\stereo_fx_bg.png",
               "dtblkfx\skins\Dark\stereo_fx_bg.png", "dtblkfx\skins\Warm\stereo_fx_bg.png",
               "licenses\DtBlkFix-LICENSE.txt", "licenses\FFTW-COPYING.txt",
               "licenses\JUCE-LICENSE.md", "licenses\VST3_SDK-LICENSE.txt",
               "licenses\THIRD_PARTY_NOTICES.md") {
  if (-not $names.Contains($f)) { throw "missing from the pack: $f" }
}
$bytes = [System.IO.File]::ReadAllBytes($dst)
$latin1 = [System.Text.Encoding]::GetEncoding(28591)
$head = $latin1.GetString([System.IO.File]::ReadAllBytes($pack), 0, 64)
if ($latin1.GetString($bytes).IndexOf($head, [System.StringComparison]::Ordinal) -lt 0) {
  throw "the pack isn't in $dst"
}

Write-Host "packaged $dst ($([math]::Round($bytes.Length / 1MB, 1)) MB)"
