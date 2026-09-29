; 脚本由 Inno Setup 脚本向导 生成！
; 有关创建 Inno Setup 脚本文件的详细资料请查阅帮助文档！

#define MyAppVersion "0.0.1"
#define MyAppName "自动压片机 v"+ MyAppVersion
#define MyAppPublisher "上海新诺仪器集团有限公司"
#define MyAppURL "http://www.sh-xinnuo.com.cn/"
#define MyAppExeName "ztable_press_bot.exe"

[Setup]
; 注: AppId的值为单独标识该应用程序。
; 不要为其他安装程序使用相同的AppId值。
; (若要生成新的 GUID，可在菜单中点击 "工具|生成 GUID"。)
AppId={{AA2B7FF4-6AD7-42AC-9DF2-BBDAE770C80E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
; [Icons] 的“quicklaunchicon”条目使用 {userappdata}，而其 [Tasks] 条目具有适合 IsAdminInstallMode 的检查。
UsedUserAreasWarning=no
; 以下行取消注释，以在非管理安装模式下运行（仅为当前用户安装）。
;PrivilegesRequired=lowest
OutputDir=E:\cpp_code\ztable_press_bot_project\setup
OutputBaseFilename=table_press_bot_setup_{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode

[Files]
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\ztable_press_bot.exe"; DestDir: "{app}"; Flags: ignoreversion
;Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\data.db"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libatomic-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libgomp-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libquadmath-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libssp-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5Charts.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5SerialPort.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5Sql.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\cpp_code\ztable_press_bot_project\cmake-build-release\ztable_press_bot\plugins\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; 注意: 不要在任何共享系统文件上使用“Flags: ignoreversion”

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

