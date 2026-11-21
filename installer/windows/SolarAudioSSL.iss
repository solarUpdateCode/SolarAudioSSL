[Setup]
AppName=SolarAudioSSL VST3
AppVersion=1.0.0
AppPublisher=SolarAudio
AppPublisherURL=https://github.com/solarUpdateCode/SolarAudioSSL
DefaultDirName={commoncf64}\VST3\SolarAudioSSL.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=SolarAudioSSL_1.0.0_Windows_VST3_Setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Files]
Source: "..\..\build\SolarAudioSSL_artefacts\Release\VST3\SolarAudioSSL.vst3\*"; DestDir: "{commoncf64}\VST3\SolarAudioSSL.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
