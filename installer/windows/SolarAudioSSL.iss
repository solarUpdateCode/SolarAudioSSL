[Setup]
AppName=SolarAudioSSL
AppVersion=1.0.0
AppPublisher=SolarAudio
AppPublisherURL=https://github.com/solarUpdateCode/SolarAudioSSL
DefaultDirName={autopf}\SolarAudio\SolarAudioSSL
DefaultGroupName=SolarAudioSSL
OutputDir=..\..\dist
OutputBaseFilename=SolarAudioSSL_1.0.0_Windows_Setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Types]
Name: "full"; Description: "Full Installation (VST3 + Standalone)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin (64-bit)"; Types: full custom
Name: "standalone"; Description: "Standalone Application"; Types: full custom

[Files]
Source: "..\..\build\SolarAudioSSL_artefacts\Release\VST3\SolarAudioSSL.vst3\*"; DestDir: "{commoncf64}\VST3\SolarAudioSSL.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\build\SolarAudioSSL_artefacts\Release\Standalone\SolarAudioSSL.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\SolarAudioSSL"; Filename: "{app}\SolarAudioSSL.exe"; Components: standalone
Name: "{group}\Uninstall SolarAudioSSL"; Filename: "{uninstallexe}"
