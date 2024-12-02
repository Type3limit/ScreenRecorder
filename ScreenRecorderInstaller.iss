; �ű��� Inno Setup �ű��� ���ɣ�
; �йش��� Inno Setup �ű��ļ�����ϸ��������İ����ĵ���

#define MyAppName "ScreenRecorder"
#define MyAppVersion "1.0"
#define MyAppPublisher "�ɶ�����ʢ�е����������޹�˾"
#define MyAppExeName "screenRecorder.exe"

[Setup]
; ע: AppId��ֵΪ������ʶ��Ӧ�ó���
; ��ҪΪ������װ����ʹ����ͬ��AppIdֵ��
; (��Ҫ�����µ� GUID�����ڲ˵��е�� "����|���� GUID"��)
AppId={{B1F475AD-C5F9-4046-9056-2BCD9C9744F5}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=C:\Users\58226\Documents\License_Agreement.rtf
; �Ƴ������У����ڹ���װģʽ�����У�Ϊ�����û���װ����
PrivilegesRequired=lowest
OutputDir=C:\Users\58226\Desktop
OutputBaseFilename=ScreenRecorderInstallerForWindows
SetupIconFile=G:\Code\screenRecord-git\ScreenRecorder\recording.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Default.isl"


[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "G:\Code\screenRecord-git\ScreenRecorder\bin\windows\Release\screenRecorder.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "G:\Code\screenRecord-git\ScreenRecorder\bin\windows\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; ע��: ��Ҫ���κι���ϵͳ�ļ���ʹ�á�Flags: ignoreversion��

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Classes\ScreenRecorder"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\ScreenRecorder"; ValueType: string; ValueName: ""; ValueData: "URL:ScreenRecorder Protocol"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\ScreenRecorder\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},1"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\ScreenRecorder\Shell"; ValueType: none; ValueName: ""; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\ScreenRecorder\Shell\Open"; ValueType: none; ValueName: ""; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\ScreenRecorder\Shell\Open\Command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "cmd.exe"; Parameters: "/C taskkill /IM {#MyAppExeName} /F"; Flags: runhidden

[Code]  
// �Զ��庯�����ж�����Ƿ����У�����Ϊ��Ҫ�жϵ������exe����
function CheckSoftRun(strExeName: String): Boolean;
// ��������
var ErrorCode: Integer;
var bRes: Boolean;
var strFileContent: AnsiString;
var strTmpPath: String;  // ��ʱĿ¼
var strTmpFile: String;  // ��ʱ�ļ����������������ݽ��
var strCmdFind: String;  // �����������
var strCmdKill: String;  // ��ֹ�������
begin
  strTmpPath := GetTempDir();
  strTmpFile := Format('%sfindSoftRes.txt', [strTmpPath]);
  strCmdFind := Format('/c tasklist /nh|find /c /i "%s" > "%s"', [strExeName, strTmpFile]);
  strCmdKill := Format('/c taskkill /f /t /im %s', [strExeName]);
  bRes := ShellExec('open', ExpandConstant('{cmd}'), strCmdFind, '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  if bRes then begin
      bRes := LoadStringFromFile(strTmpFile, strFileContent);
      strFileContent := Trim(strFileContent);
      if bRes then begin
         if StrToInt(strFileContent) > 0 then begin
            if MsgBox(ExpandConstant('{cm:checkSoftTip}'), mbConfirmation, MB_OKCANCEL) = IDOK then begin
             // ��ֹ����
             ShellExec('open', ExpandConstant('{cmd}'), strCmdKill, '', SW_HIDE, ewNoWait, ErrorCode);
             Result:= true;// ������װ
            end else begin
             Result:= false;// ��װ�����˳�
             Exit;
            end;
         end else begin
            // ���û������
            Result:= true;
            Exit;
         end;
      end;
  end;
  Result :=true;
end;

// ��ʼҳ��һ��ʱ�ж�����Ƿ�����
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if 1=CurPageID then begin
      Result := CheckSoftRun('{#MyAppExeName}');
      Exit;
  end; 
  Result:= true;
end;

// ж��ʱ�ر����
function InitializeUninstall(): Boolean;
begin
  Result := CheckSoftRun('{#MyAppExeName}');
end;


[CustomMessages]
chinesesimp.checkSoftTip=��װ�����⵽����װ������������У�%n%n���"ȷ��"��ֹ��������������������"ȡ��"��
