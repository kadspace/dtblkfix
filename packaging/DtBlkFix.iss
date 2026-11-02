; DtBlkFix installer (Inno Setup 6)
;
;   iscc /DVersion=1.0.0 /DSource=<package folder> /DOutDir=<folder> packaging\DtBlkFix.iss
;
; <package folder> is what packaging/package.ps1 makes. The VST3 goes to
; C:\Program Files\Common Files\VST3\DtBlkFix.vst3, with FFTW, the presets, images and skins inside
; the bundle (Contents\Resources\dtblkfx), where the plugin loads them. The standalone app and the
; documents go to C:\Program Files\DtBlkFix.

#ifndef Version
  #define Version "1.0.0"
#endif
#ifndef Source
  #define Source "..\build\package\DtBlkFix-" + Version + "-win64"
#endif
#ifndef OutDir
  #define OutDir "..\build\package"
#endif

[Setup]
AppId={{CF876939-AE5E-4965-BC6F-2EEEB4EF6B1D}
AppName=DtBlkFix
AppVersion={#Version}
AppVerName=DtBlkFix {#Version}
AppPublisher=kadspace
AppPublisherURL=https://github.com/kadspace/dtblkfix
DefaultDirName={autopf}\DtBlkFix
DisableProgramGroupPage=yes
LicenseFile={#Source}\LICENSE
OutputDir={#OutDir}
OutputBaseFilename=DtBlkFix-{#Version}-setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=DtBlkFix {#Version}

[Types]
Name: "full"; Description: "VST3 and standalone app"
Name: "vst3"; Description: "VST3 only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (Common Files\VST3)"; Types: full vst3 custom; Flags: fixed
Name: "standalone"; Description: "Standalone app"; Types: full

[Files]
Source: "{#Source}\DtBlkFix.vst3\*"; DestDir: "{commoncf64}\VST3\DtBlkFix.vst3"; Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#Source}\Standalone\*"; DestDir: "{app}"; Components: standalone; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#Source}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Source}\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Source}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Source}\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Source}\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\DtBlkFix"; Filename: "{app}\DtBlkFix.exe"; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\DtBlkFix.vst3"
