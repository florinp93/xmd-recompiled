; X-Men Destiny PC Port - Inno Setup Installer Script
;
; Produces a self-contained installer .exe that:
;   1. Asks the user for their game ISO
;   2. Asks for install destination
;   3. Extracts the ISO using bundled extract-xiso
;   4. Copies the game port binaries + launcher
;   5. Creates a desktop shortcut to the launcher
;
; Build with: iscc installer\xmd_installer.iss
; Requires: Inno Setup 6+ (https://jrsoftware.org/isdl.php)

#define XmdAppName "X-Men Destiny"
#define XmdAppVersion "0.1.0-alpha.3"
#define XmdAppVersionNumeric "0.1.0.3"
#define XmdAppPublisher "xmd-recompiled"
#define XmdAppExeName "xmd_launcher.exe"
#define XmdGameExeName "xmd.exe"

[Setup]
AppId={{XMD-DESTINY-PC-PORT}}
AppName={#XmdAppName}
AppVersion={#XmdAppVersion}
AppPublisher={#XmdAppPublisher}
VersionInfoVersion={#XmdAppVersionNumeric}
VersionInfoTextVersion={#XmdAppVersion}
DefaultDirName={autopf}\{#XmdAppName}
DefaultGroupName={#XmdAppName}
DisableProgramGroupPage=yes
OutputDir=..\installer_output
OutputBaseFilename=xmd_installer
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
DisableDirPage=no
DirExistsWarning=no
UninstallDisplayIcon={app}\xmd_icon.ico
SetupIconFile=assets\xmd_icon.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce

[Files]
; Launcher
Source: "build\launcher\Release\xmd_launcher.exe"; DestDir: "{app}"; Flags: ignoreversion

; Game executable + runtime DLLs (built from the port project)
Source: "build\xmd\Release\xmd.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\xmd\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion

; Icon
Source: "assets\xmd_icon.ico"; DestDir: "{app}"; Flags: ignoreversion

; Default config
Source: "assets\xmd.toml"; DestDir: "{app}"; Flags: onlyifdoesntexist

; ISO extraction tool
Source: "tools\extract-xiso\build\extract-xiso.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\{#XmdAppName}"; Filename: "{app}\{#XmdAppExeName}"; IconFilename: "{app}\xmd_icon.ico"
Name: "{group}\Uninstall {#XmdAppName}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#XmdAppName}"; Filename: "{app}\{#XmdAppExeName}"; Tasks: desktopicon; IconFilename: "{app}\xmd_icon.ico"

[Run]
; Extract the ISO into the game\ subdirectory
Filename: "{tmp}\extract-xiso.exe"; Parameters: "-x -d ""{app}\game"" ""{code:GetIsoPath}"""; \
  StatusMsg: "Extracting game files from ISO (this may take a few minutes)..."; \
  Flags: waituntilterminated runhidden; Check: IsoPathProvided

[UninstallDelete]
Type: filesandordirs; Name: "{app}\game"
Type: filesandordirs; Name: "{app}\user_data"
Type: files; Name: "{app}\xmd_runtime.log"

[Code]
var
  IsoPage: TInputFileWizardPage;
  IsoPath: String;

procedure InitializeWizard;
begin
  IsoPage := CreateInputFilePage(wpSelectDir,
    'Select Game ISO',
    'Please select your X-Men Destiny Xbox 360 ISO file.',
    'The installer will extract the game files from this ISO. You need a legally obtained copy.');
  IsoPage.Add('X-Men Destiny ISO file (*.iso)|*.iso|All files (*.*)|*.*', '.iso', '');
end;

function IsoPathProvided: Boolean;
begin
  Result := (IsoPath <> '');
end;

function GetIsoPath(Value: String): String;
begin
  Result := IsoPath;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = IsoPage.ID then
  begin
    IsoPath := IsoPage.Values[0];
    if IsoPath = '' then
    begin
      MsgBox('Please select your X-Men Destiny ISO file.', mbError, MB_OK);
      Result := False;
    end
    else if not FileExists(IsoPath) then
    begin
      MsgBox('The specified file does not exist: ' + IsoPath, mbError, MB_OK);
      Result := False;
    end
    else
    begin
      Result := True;
    end;
  end
  else
  begin
    Result := True;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    CreateDir(ExpandConstant('{app}\user_data'));
  end;
end;


