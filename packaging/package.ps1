# Put the built plugin together for release: the VST3 bundle, and a zip of it, in <out>.
#
#   package.ps1 -Build <cmake build folder> -Juce <JUCE checkout> -Out <folder> -Version <x.y.z>
#
# <out>/vst3/DtBlkFix.vst3/            the VST3 bundle, to copy into the VST3 folder as it is:
#   Contents/x86_64-win/DtBlkFix.vst3    the plugin
#   Contents/Resources/dtblkfx/          FFTW, the presets, the images and the skins
#   Contents/Resources/licenses/         DtBlkFix's license (GPLv3), THIRD_PARTY_NOTICES.md, and
#                                        the FFTW, JUCE and VST3 SDK licenses
# <out>/DtBlkFix-<version>-vst3.zip    the same bundle, zipped

param(
  [Parameter(Mandatory = $true)][string]$Build,
  [Parameter(Mandatory = $true)][string]$Juce,
  [Parameter(Mandatory = $true)][string]$Out,
  [Parameter(Mandatory = $true)][string]$Version
)
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$artefacts = Join-Path $Build "plugin/DtBlkFix_artefacts/Release"
$stage = Join-Path $Out "vst3"
$bundle = Join-Path $stage "DtBlkFix.vst3"

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item -Recurse (Join-Path $artefacts "VST3/DtBlkFix.vst3") $stage

# the licenses go with the plugin, inside the bundle
$lic = Join-Path $bundle "Contents/Resources/licenses"
New-Item -ItemType Directory -Force $lic | Out-Null
Copy-Item (Join-Path $root "LICENSE") "$lic/DtBlkFix-LICENSE.txt"
Copy-Item (Join-Path $root "THIRD_PARTY_NOTICES.md") $lic
Copy-Item (Join-Path $root "fftw/COPYING") "$lic/FFTW-COPYING.txt"
Copy-Item (Join-Path $Juce "LICENSE.md") "$lic/JUCE-LICENSE.md"
Copy-Item (Join-Path $Juce "modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt") "$lic/VST3_SDK-LICENSE.txt"

# everything the plugin needs must be there
foreach ($f in "Contents/x86_64-win/DtBlkFix.vst3",
               "Contents/Resources/dtblkfx/libfftw3f-3.dll",
               "Contents/Resources/dtblkfx/stereo_presets.txt",
               "Contents/Resources/dtblkfx/stereo_fx_bg.png",
               "Contents/Resources/dtblkfx/skins/Dark/stereo_fx_bg.png",
               "Contents/Resources/licenses/DtBlkFix-LICENSE.txt",
               "Contents/Resources/licenses/FFTW-COPYING.txt") {
  if (-not (Test-Path (Join-Path $bundle $f))) { throw "missing from the bundle: $f" }
}

$zip = Join-Path $Out "DtBlkFix-$Version-vst3.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $bundle -DestinationPath $zip
Write-Host "packaged $bundle and $zip"
Get-ChildItem -Recurse $bundle | Where-Object { -not $_.PSIsContainer } |
  ForEach-Object { "DtBlkFix.vst3/" + $_.FullName.Substring($bundle.Length + 1) }
