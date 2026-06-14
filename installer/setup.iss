; installer/setup.iss
; Golddrive Inno Setup installer script

#define MyAppName "Golddrive"
#define MyAppPublisher "Golddrive Inc."
#define MyAppURL "https://github.com/sganis/golddrive"
#define MyAppExeName "golddrive-app.exe"

; Passed via /DMyAppVersion=x.y from command line
#ifndef MyAppVersion
  #define MyAppVersion "3.0"
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

; Bundled WinFsp dependency (FUSE layer). Installed silently when absent and
; removed on uninstall only if this installer added it.
#define WinFspMsi "winfsp-2.1.25156.msi"

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
; Use "x64" (not the 6.3-only "x64compatible"): the AppVeyor VS2022 image ships
; an Inno Setup that may predate 6.3, and `choco install innosetup` skips the
; upgrade when one is already present. "x64" compiles on every Inno Setup 6.x and
; is the correct constraint since this installer ships only x64 binaries.
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
; Minimum Windows 10 / Server 2016. A Windows 11 install-time gate isn't viable:
; the AppVeyor CI image that runs the silent-install smoke test reports a build
; below 20348, so any higher floor fails CI. The Windows 11 modernization lives
; in the dependencies (WinFsp 2.x) and the compiler target, not an installer block.
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
Source: "{#BuildDir}\BouncyCastle.Cryptography.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Microsoft.Bcl.AsyncInterfaces.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Microsoft.Xaml.Behaviors.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.Buffers.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.Formats.Asn1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.Memory.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.Numerics.Vectors.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.Runtime.CompilerServices.Unsafe.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.Threading.Tasks.Extensions.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\System.ValueTuple.dll"; DestDir: "{app}"; Flags: ignoreversion

; Default config (don't overwrite existing user config)
Source: "config.json"; DestDir: "{localappdata}\Golddrive"; Flags: onlyifdoesntexist

; Documentation
Source: "help.md"; DestDir: "{app}"; Flags: ignoreversion

; SSH tools (from vendor)
Source: "{#SshDir}\ssh.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SshDir}\ssh-keygen.exe"; DestDir: "{app}"; Flags: ignoreversion

; Bundled WinFsp installer. Kept in {app} so the uninstaller can call
; "msiexec /x" on the exact package we installed (see [Code]).
Source: "..\vendor\winfsp\{#WinFspMsi}"; DestDir: "{app}"; Flags: ignoreversion

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
var
  { Whether WinFsp was already present before this install ran. Captured up
    front so the uninstaller knows if removing WinFsp is our responsibility. }
  WinFspWasPresent: Boolean;

function IsWinFspInstalled: Boolean;
var
  ServicePath: String;
begin
  Result := RegQueryStringValue(HKLM, 'SYSTEM\CurrentControlSet\Services\WinFsp.Launcher', 'ImagePath', ServicePath);
end;

function InitializeSetup: Boolean;
begin
  WinFspWasPresent := IsWinFspInstalled;
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  MsiPath: String;
begin
  { Runs after files are copied but before the postinstall "Launch Golddrive"
    Run entry, so WinFsp is ready before the app starts. }
  if CurStep = ssPostInstall then
  begin
    if not WinFspWasPresent then
    begin
      MsiPath := ExpandConstant('{app}\{#WinFspMsi}');
      if Exec('msiexec.exe', '/i "' + MsiPath + '" /qn /norestart', '',
              SW_SHOW, ewWaitUntilTerminated, ResultCode)
         and ((ResultCode = 0) or (ResultCode = 3010)) then
      begin
        { Record that WinFsp is ours, so uninstall can remove it. HKLM64 so the
          value lands in the same 64-bit-view key as the Registry-section
          InstallDir entry and gets cleaned up by its uninsdeletekey flag. }
        RegWriteDWordValue(HKLM64, 'Software\Golddrive', 'InstalledWinFsp', 1);
      end
      else
      begin
        MsgBox('Golddrive was installed, but its bundled WinFsp dependency could '
             + 'not be installed automatically (error ' + IntToStr(ResultCode) + ').'
             + #13#10 + #13#10
             + 'Golddrive cannot mount drives until WinFsp is present. You can '
             + 'install it manually from:' + #13#10
             + 'https://github.com/winfsp/winfsp/releases',
               mbError, MB_OK);
      end;
    end;
  end;
end;

procedure UnmountGoldDrives;
var
  ExitCode: Integer;
  I: Integer;
begin
  { Golddrive only uses drive letters G-Z by convention.
    net use /d /y silently ignores drives that are not mapped,
    so this is safe even if some letters are unused. }
  for I := Ord('G') to Ord('Z') do
  begin
    Exec('net.exe', 'use ' + Chr(I) + ': /d /y', '', SW_HIDE, ewWaitUntilTerminated, ExitCode);
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
  ConfigDir: String;
  MsiPath: String;
  InstalledWinFsp: Cardinal;
begin
  if CurUninstallStep = usUninstall then
  begin
    UnmountGoldDrives;

    { Remove WinFsp only if this installer added it (it was absent beforehand).
      A pre-existing WinFsp is left alone so we don't break other software that
      relies on it (sshfs-win, rclone mounts, etc.). Runs before our files are
      deleted, so the bundled MSI is still on disk for "msiexec /x". }
    if RegQueryDWordValue(HKLM64, 'Software\Golddrive', 'InstalledWinFsp', InstalledWinFsp)
       and (InstalledWinFsp = 1) then
    begin
      MsiPath := ExpandConstant('{app}\{#WinFspMsi}');
      if FileExists(MsiPath) then
        Exec('msiexec.exe', '/x "' + MsiPath + '" /qn /norestart', '',
             SW_SHOW, ewWaitUntilTerminated, ResultCode);
    end;

    { Clean up user config directory }
    ConfigDir := ExpandConstant('{localappdata}\Golddrive');
    if DirExists(ConfigDir) then
    begin
      if MsgBox('Remove Golddrive configuration and logs?' + #13#10 + ConfigDir,
                 mbConfirmation, MB_YESNO) = IDYES then
      begin
        DelTree(ConfigDir, True, True, True);
      end;
    end;
  end;
end;
