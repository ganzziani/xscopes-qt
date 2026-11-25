##############################################################################################
# XScopes Interface - Windows installer (NSIS 3.x, Unicode)
#
# Do not run makensis on this file by hand: packaging\windows\build-installer.bat builds the
# release binary, stages the payload with windeployqt and passes everything below in.
#
# Required defines:
#   APPVERSION   2.0.0                 display version
#   APPVERSION4  2.0.0.0               four numbers, for the installer's own version resource
#   REPO         <repo root>           for gpl.txt and Bitmaps\GT-icon.ico
#   PAYLOAD      <staging dir>         core\ (exe, Qt DLLs, licences, ReadMe)
#   OUTFILE      <installer.exe>       where to write the result
# Optional defines:
#   SCOPE        machine (default: Program Files, all users, asks for admin)
#                user    (per-user install under %LOCALAPPDATA%\Programs, no admin)
#
# The payload directory doubles as a portable build (unzip it and run xscope.exe): everything
# the installer writes is either in the payload or the uninstaller that it generates itself.
##############################################################################################

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

!ifndef APPVERSION
    !error "APPVERSION is not defined - build with packaging\windows\build-installer.bat"
!endif
!ifndef APPVERSION4
    !error "APPVERSION4 is not defined - build with packaging\windows\build-installer.bat"
!endif
!ifndef PAYLOAD
    !error "PAYLOAD is not defined - build with packaging\windows\build-installer.bat"
!endif
!ifndef OUTFILE
    !error "OUTFILE is not defined - build with packaging\windows\build-installer.bat"
!endif
!ifndef REPO
    !define REPO "${__FILEDIR__}\..\.."
!endif
!ifndef SCOPE
    !define SCOPE "machine"
!endif

!define APP_NAME        "XScopes Interface"
!define APP_EXE         "xscope.exe"
!define APP_PUBLISHER   "Gabotronics"
!define APP_URL         "https://gabotronics.com/"
!define APP_COPYRIGHT   "Copyright (C) 2013-2026 Gabotronics"
!define MANUAL_URL      "https://www.gabotronics.com/download/xscopes/xscopes-manual.pdf"
!define UNINST_KEY      "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define APP_PATH_KEY    "Software\Microsoft\Windows\CurrentVersion\App Paths\${APP_EXE}"

# --- per-machine (default) or per-user -------------------------------------------------------
!if "${SCOPE}" == "user"
    RequestExecutionLevel user
    !define SHELL_CONTEXT "current"
    !define REG_ROOT "HKCU"
    !define DEFAULT_INSTDIR "$LOCALAPPDATA\Programs\Gabotronics\${APP_NAME}"
    !define SCOPE_TEXT "for the current user only"
!else
    RequestExecutionLevel admin
    !define SHELL_CONTEXT "all"
    !define REG_ROOT "HKLM"
    !define DEFAULT_INSTDIR "$PROGRAMFILES64\Gabotronics\${APP_NAME}"
    !define SCOPE_TEXT "for all users"
!endif

# --- optional Authenticode signing ------------------------------------------------------------
# build-installer.bat writes signing.nsh next to this file when SIGNCERT or SIGNTHUMB is set.
# It defines ONE command, "signtool sign ... "%1"", and hands it to both hooks: !finalize runs
# it on the finished installer, !uninstfinalize on the uninstaller image that NSIS builds at
# compile time and embeds in the installer (so the uninstaller lands on the target already
# signed - that is the file Windows Defender's heuristics look at).
!if /FileExists "${__FILEDIR__}\signing.nsh"
    !include "${__FILEDIR__}\signing.nsh"
!endif

# --- installer identity ----------------------------------------------------------------------
SetCompressor /SOLID lzma
Name "${APP_NAME} ${APPVERSION}"
Caption "${APP_NAME} ${APPVERSION} Setup"
BrandingText "${APP_NAME} ${APPVERSION} - ${SCOPE_TEXT}"
OutFile "${OUTFILE}"
Icon "${REPO}\Bitmaps\GT-icon.ico"
UninstallIcon "${REPO}\Bitmaps\GT-icon.ico"
InstallDir "${DEFAULT_INSTDIR}"
CRCCheck on

VIProductVersion "${APPVERSION4}"
VIAddVersionKey /LANG=1033 "ProductName"     "${APP_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion"  "${APPVERSION}"
VIAddVersionKey /LANG=1033 "FileDescription" "${APP_NAME} Setup"
VIAddVersionKey /LANG=1033 "FileVersion"     "${APPVERSION4}"
VIAddVersionKey /LANG=1033 "CompanyName"     "${APP_PUBLISHER}"
VIAddVersionKey /LANG=1033 "LegalCopyright"  "${APP_COPYRIGHT}"

# --- look and feel ---------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_ICON "${REPO}\Bitmaps\GT-icon.ico"
!define MUI_UNICON "${REPO}\Bitmaps\GT-icon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${__FILEDIR__}\header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${__FILEDIR__}\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${__FILEDIR__}\welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${__FILEDIR__}\welcome.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC

Var StartMenuFolder

InstType "Full"
InstType "Minimal"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${REPO}\gpl.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "Gabotronics"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${REG_ROOT}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Start ${APP_NAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchApplication
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\ReadMe.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show the ReadMe file"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

# --- helpers ---------------------------------------------------------------------------------
# A running copy maps its Qt DLLs, which then cannot be replaced. Silent installs (upgrades
# from a script, unattended deployments) close it without asking.
!macro CheckRunningBody
    nsExec::ExecToStack 'cmd /c tasklist /FI "IMAGENAME eq ${APP_EXE}" /NH'
    Pop $0
    Pop $1
    ${If} $1 ${Contains} "${APP_EXE}"
        ; Silent means an unattended run (an upgrade from a script): close it unasked.
        ${IfNot} ${Silent}
            MessageBox MB_YESNO|MB_ICONQUESTION "${APP_NAME} is running. Close it and continue?" IDNO checkAbort
        ${EndIf}
        DetailPrint "Closing ${APP_NAME}..."
        nsExec::ExecToStack 'cmd /c taskkill /F /IM ${APP_EXE} /T'
        Pop $0
        Pop $1
        Sleep 1500
        Goto checkDone
checkAbort:
        Abort
    ${EndIf}
checkDone:
!macroend

Function CheckRunning
    !insertmacro CheckRunningBody
FunctionEnd

Function un.CheckRunning
    !insertmacro CheckRunningBody
FunctionEnd

; The finish page's "Start" box. The per-machine installer runs elevated, and a plain Exec would
; start the application elevated too (its settings would go to the account that typed the admin
; password). Explorer starts it as the signed-in user, unelevated.
Function LaunchApplication
    Exec '"$WINDIR\explorer.exe" "$INSTDIR\${APP_EXE}"'
FunctionEnd

Function .onInit
    SetShellVarContext ${SHELL_CONTEXT}
    SetRegView 64
    ; An upgrade keeps the folder the user chose last time instead of the default, unless
    ; /D=<dir> asked for another one (NSIS has already put that in $INSTDIR). Read here, not
    ; with InstallDirRegKey: that reads the 32-bit view, where HKLM\Software is redirected.
    ${If} $INSTDIR == "${DEFAULT_INSTDIR}"
        ReadRegStr $0 ${REG_ROOT} "${UNINST_KEY}" "InstallLocation"
        ${If} $0 != ""
            StrCpy $INSTDIR $0
        ${EndIf}
    ${EndIf}
    Call CheckRunning
FunctionEnd

Function un.onInit
    SetShellVarContext ${SHELL_CONTEXT}
    SetRegView 64
    Call un.CheckRunning
FunctionEnd

# --- sections --------------------------------------------------------------------------------
Section "!${APP_NAME} (required)" SecCore
    SectionIn RO
    SetShellVarContext ${SHELL_CONTEXT}
    SetRegView 64

    ; Remove a previous version found in the same folder first: the Qt DLLs and plugin
    ; directories differ between Qt releases, and leftovers would still be loaded.
    ${If} ${FileExists} "$INSTDIR\uninstall.exe"
        DetailPrint "Removing the previous installation in $INSTDIR..."
        ExecWait '"$INSTDIR\uninstall.exe" /S _?=$INSTDIR'
        Delete "$INSTDIR\uninstall.exe"
    ${EndIf}

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File /r "${PAYLOAD}\core\*.*"
SectionEnd

Section "Start menu shortcuts" SecStartMenu
    SectionIn 1 2
    SetShellVarContext ${SHELL_CONTEXT}
    SetOutPath "$INSTDIR"
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
        ; The manual is not shipped: an Internet shortcut opens the current one on the website.
        WriteINIStr "$SMPROGRAMS\$StartMenuFolder\${APP_NAME} Manual.url" "InternetShortcut" "URL" "${MANUAL_URL}"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "Desktop shortcut" SecDesktop
    SectionIn 1
    SetShellVarContext ${SHELL_CONTEXT}
    SetOutPath "$INSTDIR"
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

# Registry bookkeeping, written once the files are on disk so the reported size is right.
Section "-Registry"
    SetShellVarContext ${SHELL_CONTEXT}
    SetRegView 64
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0

    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "DisplayVersion"  "${APPVERSION}"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\${APP_EXE},0"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "Publisher"       "${APP_PUBLISHER}"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "URLInfoAbout"    "${APP_URL}"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "HelpLink"        "${APP_URL}"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr ${REG_ROOT} "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegDWORD ${REG_ROOT} "${UNINST_KEY}" "EstimatedSize" "$0"
    WriteRegDWORD ${REG_ROOT} "${UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${REG_ROOT} "${UNINST_KEY}" "NoRepair" 1

    WriteRegStr ${REG_ROOT} "${APP_PATH_KEY}" ""     "$INSTDIR\${APP_EXE}"
    WriteRegStr ${REG_ROOT} "${APP_PATH_KEY}" "Path" "$INSTDIR"
SectionEnd

Section "Uninstall"
    SetShellVarContext ${SHELL_CONTEXT}
    SetRegView 64

    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\${APP_NAME} Manual.url"
    Delete "$SMPROGRAMS\$StartMenuFolder\${APP_NAME} Manual.lnk"   ; installed by early test builds that shipped the PDF
    Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall ${APP_NAME}.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"
    RMDir "$SMPROGRAMS\Gabotronics"
    Delete "$DESKTOP\${APP_NAME}.lnk"

    DeleteRegKey ${REG_ROOT} "${APP_PATH_KEY}"
    DeleteRegKey ${REG_ROOT} "${UNINST_KEY}"

    ; Settings live in the registry (QSettings), so by default they survive an uninstall.
    StrCpy $0 "0"
    IfSilent uninstallFiles
    MessageBox MB_YESNO|MB_ICONQUESTION "Also remove the ${APP_NAME} settings and window state?" IDNO uninstallFiles
    StrCpy $0 "1"
uninstallFiles:
    ${If} $0 == "1"
        DeleteRegKey HKCU "Software\Gabotronics\${APP_NAME}"
        DeleteRegKey HKCU "Software\Gabotronics Xscope"
    ${EndIf}

    RMDir /r "$INSTDIR"
SectionEnd

# --- component descriptions (must come after the sections) ------------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}      "The application, the Qt runtime it needs, libusb and the licence files. Required."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} "A Start menu entry, with shortcuts to the online manual and to the uninstaller."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop}   "A shortcut on the desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
