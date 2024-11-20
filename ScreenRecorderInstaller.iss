; 脚本由 Inno Setup 脚本向导 生成！
; 有关创建 Inno Setup 脚本文件的详细资料请查阅帮助文档！

#define MyAppName "ScreenRecorder"
#define MyAppVersion "1.0"
#define MyAppPublisher "成都东方盛行电子责任有限公司"
#define MyAppExeName "screenRecorder.exe"

[Setup]
; 注: AppId的值为单独标识该应用程序。
; 不要为其他安装程序使用相同的AppId值。
; (若要生成新的 GUID，可在菜单中点击 "工具|生成 GUID"。)
AppId={{B1F475AD-C5F9-4046-9056-2BCD9C9744F5}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=C:\Users\58226\Documents\License_Agreement.rtf
; 移除以下行，以在管理安装模式下运行（为所有用户安装）。
PrivilegesRequired=lowest
OutputDir=C:\Users\58226\Desktop
OutputBaseFilename=ScreenRecorderInstaller
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
; 注意: 不要在任何共享系统文件上使用“Flags: ignoreversion”

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

