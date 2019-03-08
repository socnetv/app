#define APPTITLE "Social Network Visualizer"
#define APPSHORT "SocNetV"
#define RELEASEFOLDER "release\"
#define EXECUTABLE APPSHORT + ".exe"
#define NUMERICVERSION GetFileVersion(RELEASEFOLDER+EXECUTABLE)
#define VERSION "2.5" 
#define URL "https://socnetv.org"
#define COPYRIGHT "2005-2018 " + URL

[Setup]
AllowNoIcons=yes
AppName={#APPTITLE}
AppId={#APPSHORT}
AppPublisher={#APPTITLE}
AppPublisherURL={#URL}
AppSupportURL={#URL}
AppUpdatesURL={#URL}
AppContact=info@socnetv.org
AppVerName={#APPTITLE} {#VERSION}
Compression=lzma/ultra
DefaultDirName={pf}\{#APPSHORT}
DefaultGroupName={#APPTITLE}
DisableProgramGroupPage=true
LicenseFile={#RELEASEFOLDER}LICENSE.txt
InternalCompressLevel=ultra
OutputBaseFilename={#APPSHORT}-{#VERSION}-installer
OutputDir=.
OutputManifestFile=Setup-Manifest.txt
ShowLanguageDialog=no
SolidCompression=yes
VersionInfoProductName={#APPTITLE}
VersionInfoCompany={#URL}
VersionInfoCopyright=Copyright (C) {#COPYRIGHT}
VersionInfoDescription={#APPTITLE} Setup
VersionInfoTextVersion={#VERSION}
VersionInfoVersion={#NUMERICVERSION}
WizardImageFile=compiler:WizModernImage-IS.bmp
WizardSmallImageFile=compiler:WizModernSmallImage-IS.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "serbian"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: {#RELEASEFOLDER}*; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#APPTITLE}"; Filename: "{app}\{#EXECUTABLE}"
Name: "{group}\{cm:UninstallProgram,{#APPTITLE}}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#APPTITLE}"; Filename: "{app}\{#EXECUTABLE}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#APPTITLE}"; Filename: "{app}\{#EXECUTABLE}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#EXECUTABLE}"; Description: "{cm:LaunchProgram,{#APPTITLE}}"; Flags: nowait postinstall skipifsilent

