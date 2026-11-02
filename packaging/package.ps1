# Put the built plugin together for release: a folder and a zip in <out>.
#
#   package.ps1 -Build <cmake build folder> -Juce <JUCE checkout> -Out <folder> -Version <x.y.z>
#
# DtBlkFix-<version>-win64/
#   DtBlkFix.vst3/                    the VST3 bundle; Contents/Resources/dtblkfx holds FFTW, the
#                                     presets, the images and the skins
#   Standalone/DtBlkFix.exe           the standalone app, with its own dtblkfx folder
#   README.md, CHANGELOG.md, LICENSE, THIRD_PARTY_NOTICES.md
#   licenses/                         FFTW, JUCE and VST3 SDK licenses

param(
  [Parameter(Mandatory = $true)][string]$Build,
  [Parameter(Mandatory = $true)][string]$Juce,
  [Parameter(Mandatory = $true)][string]$Out,
  [Parameter(Mandatory = $true)][string]$Version
)
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$artefacts = Join-Path $Build "plugin/DtBlkFix_artefacts/Release"
$name = "DtBlkFix-$Version-win64"
$dir = Join-Path $Out $name

if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
New-Item -ItemType Directory -Force $dir, "$dir/Standalone", "$dir/licenses" | Out-Null

Copy-Item -Recurse (Join-Path $artefacts "VST3/DtBlkFix.vst3") $dir
Copy-Item (Join-Path $artefacts "Standalone/DtBlkFix.exe") "$dir/Standalone"
Copy-Item -Recurse (Join-Path $artefacts "Standalone/dtblkfx") "$dir/Standalone"

foreach ($f in "README.md", "CHANGELOG.md", "LICENSE", "THIRD_PARTY_NOTICES.md") {
  Copy-Item (Join-Path $root $f) $dir
}
Copy-Item (Join-Path $root "fftw/COPYING") "$dir/licenses/FFTW-COPYING.txt"
Copy-Item (Join-Path $Juce "LICENSE.md") "$dir/licenses/JUCE-LICENSE.md"
Copy-Item (Join-Path $Juce "modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt") "$dir/licenses/VST3_SDK-LICENSE.txt"

# everything the plugin needs must be there
foreach ($f in "DtBlkFix.vst3/Contents/x86_64-win/DtBlkFix.vst3",
               "DtBlkFix.vst3/Contents/Resources/dtblkfx/libfftw3f-3.dll",
               "DtBlkFix.vst3/Contents/Resources/dtblkfx/stereo_presets.txt",
               "DtBlkFix.vst3/Contents/Resources/dtblkfx/stereo_fx_bg.png",
               "DtBlkFix.vst3/Contents/Resources/dtblkfx/skins/Dark/stereo_fx_bg.png",
               "Standalone/dtblkfx/libfftw3f-3.dll") {
  if (-not (Test-Path (Join-Path $dir $f))) { throw "missing from the package: $f" }
}

$zip = Join-Path $Out "$name.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $dir -DestinationPath $zip
Write-Host "packaged $zip"
Get-ChildItem -Recurse $dir | Where-Object { -not $_.PSIsContainer } |
  ForEach-Object { $_.FullName.Substring($dir.Length + 1) }
