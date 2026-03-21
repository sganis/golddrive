; installer/setup.iss
; Golddrive Inno Setup installer script

#define MyAppName "Golddrive"
#define MyAppPublisher "Golddrive Inc."
#define MyAppURL "https://github.com/sganis/golddrive"
#define MyAppExeName "golddrive-app.exe"

; Passed via /DMyAppVersion=x.y from command line
#ifndef MyAppVersion
  #define MyAppVersion "2.5"
#endif

; Passed via /DMyPlatform=x64 from command line
#ifndef MyPlatform
  #define MyPlatform "x64"
#endif

; Passed via /DMyConfiguration=Release from command line
#ifndef MyConfiguration
  #define MyConfiguration "Release"
#endif

; Build output relative to this .iss file (installer/)
#define BuildDir "..\build\" + MyConfiguration + "\" + MyPlatform
#define SshDir "..\vendor\openssh\" + MyPlatform

[Setup]
AppId={{EFCA9EFA-7F65-4C74-A65D-88092D67F41A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir={#BuildDir}
OutputBaseFilename=golddrive-{#MyAppVersion}-{#MyPlatform}-setup
SetupIconFile=..\src\app\golddrive.ico
UninstallDisplayIcon={app}\golddrive.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
DisableProgramGroupPage=yes
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; CLI filesystem driver
Source: "{#BuildDir}\golddrive.exe"; DestDir: "{app}"; Flags: ignoreversion

; WPF app and dependencies
Source: "{#BuildDir}\golddrive-app.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\golddrive-app.exe.config"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\app\golddrive.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\NLog.config"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\MaterialDesignColors.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\MaterialDesignThemes.Wpf.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Newtonsoft.Json.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\NLog.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Renci.SshNet.dll"; DestDir: "{app}"; Flags: ignoreversion

; Default config (don't overwrite existing user config)
Source: "config.json"; DestDir: "{localappdata}\Golddrive"; Flags: onlyifdoesntexist

; Documentation
Source: "help.md"; DestDir: "{app}"; Flags: ignoreversion

; SSH tools (from vendor)
Source: "{#SshDir}\ssh.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SshDir}\ssh-keygen.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\golddrive.ico"
Name: "{group}\{#MyAppName} Help"; Filename: "notepad.exe"; Parameters: """{app}\help.md"""
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\golddrive.ico"
Name: "{autoprograms}\{#MyAppName} Help"; Filename: "notepad.exe"; Parameters: """{app}\help.md"""

[Registry]
; Register golddrive.exe with WinFsp Launcher service
Root: HKLM; Subkey: "Software\WOW6432Node\WinFsp\Services\golddrive"; ValueType: string; ValueName: "Executable"; ValueData: "{app}\golddrive.exe"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\WOW6432Node\WinFsp\Services\golddrive"; ValueType: string; ValueName: "CommandLine"; ValueData: "%2 %1"
Root: HKLM; Subkey: "Software\WOW6432Node\WinFsp\Services\golddrive"; ValueType: string; ValueName: "Security"; ValueData: "D:P(A;;RPWPLC;;;WD)"
Root: HKLM; Subkey: "Software\WOW6432Node\WinFsp\Services\golddrive"; ValueType: string; ValueName: "RunAs"; ValueData: "."
Root: HKLM; Subkey: "Software\WOW6432Node\WinFsp\Services\golddrive"; ValueType: dword; ValueName: "JobControl"; ValueData: "1"
Root: HKLM; Subkey: "Software\WOW6432Node\WinFsp\Services\golddrive"; ValueType: dword; ValueName: "Credentials"; ValueData: "0"

; Disable SMB directory cache (improves SSHFS responsiveness)
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Services\LanmanWorkstation\Parameters"; ValueType: dword; ValueName: "directorycachelifetime"; ValueData: "0"

; Store install directory for app to find CLI
Root: HKLM; Subkey: "Software\{#MyAppName}"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"; Flags: uninsdeletekey

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent runascurrentuser

[Code]
function IsWinFspInstalled: Boolean;
var
  ServicePath: String;
begin
  Result := RegQueryStringValue(HKLM, 'SYSTEM\CurrentControlSet\Services\WinFsp.Launcher', 'ImagePath', ServicePath);
end;

function InitializeSetup: Boolean;
begin
  Result := True;
  if not IsWinFspInstalled then
  begin
    MsgBox('WinFsp is required but not installed.' + #13#10 + #13#10 +
           'Please download and install WinFsp from:' + #13#10 +
           'https://github.com/winfsp/winfsp/releases' + #13#10 + #13#10 +
           'Then run this installer again.', mbError, MB_OK);
    Result := False;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usUninstall then
  begin
    Exec('net.exe', 'use * /d /y', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;
