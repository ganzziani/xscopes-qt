@echo off
setlocal EnableExtensions

rem ============================================================================
rem  XScopes Interface - build the Windows installer
rem
rem  Builds the release executable with the Qt 6 MSVC 64-bit kit, stages it with
rem  windeployqt together with the licence files, and packs the
rem  result with NSIS.
rem
rem  Prerequisites
rem    * Qt 6.8 or newer for MSVC 2022 64-bit. Default C:\Qt\6.12.0\msvc2022_64,
rem      override with the QTDIR environment variable.
rem    * Visual Studio 2022 with the C++ x64 tools (found through vswhere).
rem    * NSIS 3.x (https://nsis.sourceforge.io/Download). Found on PATH or in
rem      C:\Program Files (x86)\NSIS, override with the MAKENSIS variable.
rem    * Python 3 (py.exe or python.exe on PATH, or the PYTHON variable) and the
rem      kit's Qt documentation (C:\Qt\Docs\Qt-<version>, or the QTDOCS variable):
rem      qt-third-party.py writes licenses\Qt-third-party.txt from it.
rem
rem  Usage
rem    build-installer.bat [options]
rem
rem      /machine      install for all users, into Program Files (default; asks
rem                    for admin rights when the user runs it)
rem      /user         per-user installer under %LOCALAPPDATA%\Programs, no admin
rem      /nobuild      reuse the executable in the build directory, skip
rem                    qmake/nmake (for a quick re-pack of the installer)
rem      /clean        delete the build directory first
rem      /out dir      where to write the installer; defaults to the build
rem                    directory plus \installer
rem      /dirty        allow uncommitted changes (test builds; the source archive is
rem                    then not the source of the binary, so never release it)
rem      /h /help      this text
rem
rem  Environment
rem      QTDIR             Qt kit directory (bin\qmake.exe, bin\windeployqt.exe)
rem      MAKENSIS          makensis.exe, if it is not on PATH
rem      PYTHON            python.exe, if neither py.exe nor python.exe is on PATH
rem      QTDOCS            Qt documentation root (default: the Docs folder of the Qt install)
rem      XSCOPE_BUILD_DIR  build directory (default %TEMP%\xscopes-qt-build\win64-release)
rem
rem  Code signing (optional; see packaging\README.md)
rem      SIGNCERT          certificate subject name, e.g. "Gabotronics S.A."
rem      SIGNTHUMB         SHA-1 thumbprint of the certificate, used instead of SIGNCERT
rem      TIMESTAMP_URL     RFC 3161 timestamp server, default http://timestamp.digicert.com
rem    With a certificate configured, signtool from the Windows SDK signs xscope.exe, the
rem    installer, and the uninstaller image that the installer writes on the target.
rem
rem  The build directory is always outside the source tree. The staged payload
rem  in %XSCOPE_BUILD_DIR%\stage is a working portable copy: zipping its core\
rem  folder gives the "no installer" build.
rem ============================================================================

set "SCRIPT_DIR=%~dp0"
for %%i in ("%SCRIPT_DIR%..\..") do set "REPO=%%~fi"

set "SCOPE=machine"
set "DO_BUILD=1"
set "DO_CLEAN=0"
set "OUTDIR="
set "ALLOW_DIRTY="

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="/?" goto usage
if /i "%~1"=="/h" goto usage
if /i "%~1"=="/help" goto usage
if /i "%~1"=="/machine" (set "SCOPE=machine" & shift & goto parse_args)
if /i "%~1"=="/user" (set "SCOPE=user" & shift & goto parse_args)
if /i "%~1"=="/nobuild" (set "DO_BUILD=0" & shift & goto parse_args)
if /i "%~1"=="/clean" (set "DO_CLEAN=1" & shift & goto parse_args)
if /i "%~1"=="/dirty" (set "ALLOW_DIRTY=--allow-dirty" & shift & goto parse_args)
if /i "%~1"=="/out" (
    if "%~2"=="" (echo Missing directory after /out & exit /b 2)
    set "OUTDIR=%~2"
    shift & shift & goto parse_args
)
echo Unknown option: %~1
echo.
goto usage

:args_done

rem --- version, from VERSION in the .pro (the one place it is written down) ------
set "APPVERSION="
for /f "tokens=2 delims== " %%a in ('findstr /r /b /c:"VERSION *=" "%REPO%\XprotolabInterface.pro"') do set "APPVERSION=%%a"
if not defined APPVERSION (
    echo ERROR: no "VERSION = x.y.z" line in "%REPO%\XprotolabInterface.pro"
    exit /b 1
)
set "APPVERSION4=%APPVERSION%"
for /f "tokens=1,2,3 delims=." %%a in ("%APPVERSION%") do (
    if "%%c"=="" (
        echo ERROR: VERSION in the .pro must have three numbers, got "%APPVERSION%"
        exit /b 1
    )
    set "APPVERSION4=%%a.%%b.%%c.0"
)

rem --- toolchain ---------------------------------------------------------------
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINSTALL="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
)
if defined VSINSTALL if exist "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    echo ERROR: vcvars64.bat not found. Install Visual Studio 2022 with "Desktop development with C++".
    exit /b 1
)

if not defined QTDIR set "QTDIR=C:\Qt\6.12.0\msvc2022_64"
if not exist "%QTDIR%\bin\qmake.exe" (
    echo ERROR: no Qt kit at "%QTDIR%".
    echo        Set QTDIR to the msvc2022_64 directory of your Qt 6 installation.
    exit /b 1
)

rem Qt-third-party.txt is generated for every installer from the documentation of the kit's
rem Qt version (qt-third-party.py), so the notices always match the Qt DLLs in the payload.
set "QTVER="
for /f "delims=" %%v in ('"%QTDIR%\bin\qmake.exe" -query QT_VERSION') do set "QTVER=%%v"
if not defined QTDOCS for %%d in ("%QTDIR%\..\..\Docs") do set "QTDOCS=%%~fd"
if not exist "%QTDOCS%\Qt-%QTVER%\qtcore" (
    echo ERROR: no Qt %QTVER% documentation at "%QTDOCS%\Qt-%QTVER%".
    echo        Its attribution pages are the source of licenses\Qt-third-party.txt. Add the
    echo        documentation with the Qt Maintenance Tool, or set QTDOCS to the Docs folder.
    exit /b 1
)
set "PYTHON_CMD="
if defined PYTHON set "PYTHON_CMD="%PYTHON%""
if not defined PYTHON_CMD for %%i in (py.exe) do if not "%%~$PATH:i"=="" set "PYTHON_CMD="%%~$PATH:i" -3"
if not defined PYTHON_CMD for %%i in (python.exe) do if not "%%~$PATH:i"=="" set "PYTHON_CMD="%%~$PATH:i""
if defined PYTHON_CMD (
    %PYTHON_CMD% -c "import sys; sys.exit(sys.version_info < (3, 6))" >nul 2>&1 || set "PYTHON_CMD="
)
if not defined PYTHON_CMD (
    echo ERROR: Python 3 not found ^(py.exe or python.exe on PATH^), or set PYTHON to python.exe.
    exit /b 1
)

set "MAKENSIS_BIN=%MAKENSIS%"
if not defined MAKENSIS_BIN (
    for %%i in (makensis.exe) do if not "%%~$PATH:i"=="" set "MAKENSIS_BIN=%%~$PATH:i"
)
if not defined MAKENSIS_BIN if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "MAKENSIS_BIN=%ProgramFiles(x86)%\NSIS\makensis.exe"
if not defined MAKENSIS_BIN if exist "%ProgramFiles%\NSIS\makensis.exe" set "MAKENSIS_BIN=%ProgramFiles%\NSIS\makensis.exe"
if not defined MAKENSIS_BIN (
    echo ERROR: makensis.exe not found.
    echo        Install NSIS 3.x from https://nsis.sourceforge.io/Download
    echo        or set the MAKENSIS environment variable to makensis.exe.
    exit /b 1
)

rem --- optional code signing ---------------------------------------------------
set "SIGNING=0"
set "SIGNTOOL="
set "SIGN_SELECT="
if defined SIGNCERT set "SIGNING=1"
if defined SIGNTHUMB set "SIGNING=1"
if not defined TIMESTAMP_URL set "TIMESTAMP_URL=http://timestamp.digicert.com"
if "%SIGNING%"=="1" (
    rem Newest Windows SDK that has an x64 signtool.
    for /f "delims=" %%d in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do (
        if not defined SIGNTOOL if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%d\x64\signtool.exe" set "SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\%%d\x64\signtool.exe"
    )
    if not defined SIGNTOOL for %%p in (signtool.exe) do if not "%%~$PATH:p"=="" set "SIGNTOOL=%%~$PATH:p"
    if not defined SIGNTOOL (
        echo ERROR: signtool.exe not found. Install the Windows SDK, or put signtool on PATH.
        exit /b 1
    )
    if defined SIGNTHUMB (
        set "SIGN_SELECT=/sha1 %SIGNTHUMB%"
    ) else (
        rem Quoted: the subject name has spaces, and signtool would read the rest as file names.
        set SIGN_SELECT=/n "%SIGNCERT%"
    )
)

if not defined XSCOPE_BUILD_DIR set "XSCOPE_BUILD_DIR=%TEMP%\xscopes-qt-build\win64-release"
if not defined OUTDIR set "OUTDIR=%XSCOPE_BUILD_DIR%\installer"
set "STAGE=%XSCOPE_BUILD_DIR%\stage"
set "OUTFILE=%OUTDIR%\XScopes-Interface-%APPVERSION%-win64-setup.exe"
set "SRCFILE=%OUTDIR%\XScopes-Interface-%APPVERSION%-source.zip"

echo.
echo   XScopes Interface %APPVERSION% - Windows installer (%SCOPE% scope)
echo   -------------------------------------------------------------
echo   repository   %REPO%
echo   Qt kit       %QTDIR%
echo   build dir    %XSCOPE_BUILD_DIR%
echo   NSIS         %MAKENSIS_BIN%
echo   installer    %OUTFILE%
if "%SIGNING%"=="1" (
    echo   signing      %SIGN_SELECT% with "%SIGNTOOL%"
) else (
    echo   signing      none ^(set SIGNCERT or SIGNTHUMB to sign^)
)
echo.

if "%DO_CLEAN%"=="1" (
    echo   Cleaning the build directory...
    if exist "%XSCOPE_BUILD_DIR%" rmdir /s /q "%XSCOPE_BUILD_DIR%"
)

rem --- 1. release build --------------------------------------------------------
call "%VCVARS%" >nul
if errorlevel 1 (
    echo ERROR: "%VCVARS%" failed.
    exit /b 1
)

if "%DO_BUILD%"=="1" (
    echo   [1/4] Building the release executable...
    if not exist "%XSCOPE_BUILD_DIR%" mkdir "%XSCOPE_BUILD_DIR%"
    pushd "%XSCOPE_BUILD_DIR%"
    "%QTDIR%\bin\qmake.exe" "%REPO%\XprotolabInterface.pro" CONFIG+=release
    if errorlevel 1 (
        popd
        echo ERROR: qmake failed.
        exit /b 1
    )
    nmake /NOLOGO release
    if errorlevel 1 (
        popd
        echo ERROR: nmake failed.
        exit /b 1
    )
    popd
) else (
    echo   [1/4] Skipping the build, reusing "%XSCOPE_BUILD_DIR%\release\xscope.exe".
)

if not exist "%XSCOPE_BUILD_DIR%\release\xscope.exe" (
    echo ERROR: "%XSCOPE_BUILD_DIR%\release\xscope.exe" does not exist. Run without /nobuild.
    exit /b 1
)

rem --- 2. payload --------------------------------------------------------------
echo   [2/4] Staging the payload in "%STAGE%"...
if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%\core"

copy /y "%XSCOPE_BUILD_DIR%\release\xscope.exe" "%STAGE%\core\" >nul
copy /y "%REPO%\libs\win\libusb-1.0.dll" "%STAGE%\core\" >nul

pushd "%STAGE%\core"
"%QTDIR%\bin\windeployqt.exe" --release --no-translations --no-compiler-runtime --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw xscope.exe
if errorlevel 1 (
    popd
    echo ERROR: windeployqt failed.
    exit /b 1
)
popd

rem The Visual C++ runtime, deployed app-local so the target needs no vcredist and no
rem admin rights. vcvars64 sets VCToolsRedistDir; the DLLs come from the redistributable
rem folder of the toolset that built it.
set "CRT_COPIED=0"
if defined VCToolsRedistDir (
    for /d %%d in ("%VCToolsRedistDir%x64\Microsoft.VC*.CRT") do (
        copy /y "%%d\*.dll" "%STAGE%\core\" >nul
        set "CRT_COPIED=1"
    )
)
if "%CRT_COPIED%"=="0" (
    echo   WARNING: no Visual C++ runtime DLLs found under "%VCToolsRedistDir%x64".
    echo            The installed application will need the VC++ 2015-2022 x64 redistributable.
)

copy /y "%REPO%\gpl.txt" "%STAGE%\core\" >nul
rem The ReadMe names this release's source downloads, so it gets the versions filled in.
%PYTHON_CMD% -I -c "import sys; t=open(sys.argv[1],encoding='utf-8').read(); open(sys.argv[2],'w',encoding='utf-8').write(t.replace('@VERSION@',sys.argv[3]).replace('@QTVER@',sys.argv[4]))" "%SCRIPT_DIR%ReadMe.txt" "%STAGE%\core\ReadMe.txt" %APPVERSION% %QTVER%
if errorlevel 1 (
    echo ERROR: could not write ReadMe.txt.
    exit /b 1
)
xcopy /y /q /e /i "%REPO%\licenses\*" "%STAGE%\core\licenses\" >nul
echo   Writing the notices of the code inside Qt %QTVER%...
%PYTHON_CMD% -I "%SCRIPT_DIR%qt-third-party.py" --version %QTVER% --docs "%QTDOCS%" --out "%STAGE%\core\licenses\Qt-third-party.txt"
if errorlevel 1 (
    echo ERROR: qt-third-party.py failed, the payload would lack the Qt notices.
    exit /b 1
)
mkdir "%STAGE%\core\licenses\dfu-programmer"
copy /y "%REPO%\third_party\dfu-programmer\COPYING" "%STAGE%\core\licenses\dfu-programmer\" >nul
copy /y "%REPO%\third_party\dfu-programmer\AUTHORS" "%STAGE%\core\licenses\dfu-programmer\" >nul
copy /y "%REPO%\third_party\dfu-programmer\PATCHES.md" "%STAGE%\core\licenses\dfu-programmer\" >nul

rem --- 3. sanity check ---------------------------------------------------------
echo   [3/4] Checking the payload...
set "MISSING=0"
for %%f in (
    "%STAGE%\core\xscope.exe"
    "%STAGE%\core\libusb-1.0.dll"
    "%STAGE%\core\Qt6Core.dll"
    "%STAGE%\core\Qt6Gui.dll"
    "%STAGE%\core\Qt6Widgets.dll"
    "%STAGE%\core\Qt6Network.dll"
    "%STAGE%\core\Qt6SerialPort.dll"
    "%STAGE%\core\platforms\qwindows.dll"
    "%STAGE%\core\imageformats\qico.dll"
    "%STAGE%\core\gpl.txt"
    "%STAGE%\core\ReadMe.txt"
    "%STAGE%\core\licenses\README.md"
    "%STAGE%\core\licenses\Qt-third-party.txt"
    "%STAGE%\core\licenses\LGPL-2.1.txt"
    "%STAGE%\core\licenses\OFL-1.1.txt"
    "%STAGE%\core\licenses\MIT-QextSerialPort.txt"
    "%STAGE%\core\licenses\dfu-programmer\COPYING"
) do (
    if not exist %%f (
        echo   MISSING: %%f
        set "MISSING=1"
    )
)
if "%MISSING%"=="1" (
    echo ERROR: the payload is incomplete, not building an installer from it.
    exit /b 1
)

rem --- 4. installer ------------------------------------------------------------
echo   [4/4] Packing the installer with NSIS...
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem The application first: the installed xscope.exe must carry the signature, and Explorer,
rem SmartScreen and the anti-virus heuristics look at it before anything else.
if "%SIGNING%"=="1" (
    echo   Signing xscope.exe...
    "%SIGNTOOL%" sign /fd sha256 /tr "%TIMESTAMP_URL%" /td sha256 %SIGN_SELECT% "%STAGE%\core\xscope.exe"
    if errorlevel 1 (
        echo ERROR: signing "%STAGE%\core\xscope.exe" failed.
        exit /b 1
    )
)

rem NSIS signs the installer (!finalize) and the uninstaller image embedded in it
rem (!uninstfinalize) with the command lines the script writes into signing.nsh.
if "%SIGNING%"=="1" (
    > "%SCRIPT_DIR%signing.nsh" echo !define SIGN_CMD `"%SIGNTOOL%" sign /fd sha256 /tr %TIMESTAMP_URL% /td sha256 %SIGN_SELECT% "%%1"`
    >> "%SCRIPT_DIR%signing.nsh" echo !finalize '${SIGN_CMD}'
    >> "%SCRIPT_DIR%signing.nsh" echo !uninstfinalize '${SIGN_CMD}'
) else (
    if exist "%SCRIPT_DIR%signing.nsh" del "%SCRIPT_DIR%signing.nsh"
)

"%MAKENSIS_BIN%" /V2 /WX ^
    -DAPPVERSION=%APPVERSION% ^
    -DAPPVERSION4=%APPVERSION4% ^
    -DREPO="%REPO%" ^
    -DPAYLOAD="%STAGE%" ^
    -DOUTFILE="%OUTFILE%" ^
    -DSCOPE=%SCOPE% ^
    "%SCRIPT_DIR%installer.nsi"
if errorlevel 1 (
    echo ERROR: makensis failed.
    exit /b 1
)

if not exist "%OUTFILE%" (
    echo ERROR: makensis reported success but "%OUTFILE%" does not exist.
    exit /b 1
)

if "%SIGNING%"=="1" (
    "%SIGNTOOL%" verify /pa /q "%OUTFILE%"
    if errorlevel 1 (
        echo   WARNING: the installer's signature does not chain to a trusted root.
        echo            That is expected with a test certificate; with a real one this must pass.
    ) else (
        echo   Signature verified against a trusted root.
    )
)

rem --- 5. source archive ---------------------------------------------------------
rem The GPL wants the complete source next to every binary download, including the scripts that
rem build and install it, which live in this folder rather than in the repository.
%PYTHON_CMD% -I "%SCRIPT_DIR%make-source-zip.py" --repo "%REPO%" --version %APPVERSION% --out "%SRCFILE%" %ALLOW_DIRTY%
if errorlevel 1 (
    echo ERROR: no source archive. Commit the changes, or use /dirty for a test build.
    exit /b 1
)

echo.
for %%f in ("%OUTFILE%") do echo   Built %%~nxf  (%%~zf bytes)
echo   %OUTFILE%
if "%SIGNING%"=="1" echo   signed with %SIGN_SELECT% (timestamp %TIMESTAMP_URL%)
for %%f in ("%SRCFILE%") do echo   Built %%~nxf  (%%~zf bytes)
echo.
echo   Publish with the installer, in the release "v%APPVERSION%" on GitHub:
echo     %SRCFILE%
echo     qtbase-everywhere-src-%QTVER%.tar.xz, qtsvg-everywhere-src-%QTVER%.tar.xz,
echo     qtserialport-everywhere-src-%QTVER%.tar.xz  (download.qt.io/archive/qt/)
echo     libusb-1.0.30.tar.bz2  (github.com/libusb/libusb/releases)
echo.
exit /b 0

:usage
echo.
echo   XScopes Interface - build the Windows installer
echo.
echo   build-installer.bat [/machine ^| /user] [/nobuild] [/clean] [/out ^<dir^>] [/dirty]
echo.
echo     /machine   installer for all users, Program Files, needs admin (default)
echo     /user      per-user installer under %%LOCALAPPDATA%%\Programs, no admin
echo     /nobuild   reuse the existing release build, only re-pack the installer
echo     /clean     delete the build directory first (full rebuild)
echo     /out dir   where to write the installer (default ^<build dir^>\installer)
echo     /dirty     allow uncommitted changes (test builds only, never a release)
echo.
echo   Environment: QTDIR, MAKENSIS, XSCOPE_BUILD_DIR. See the header of this file.
echo.
exit /b 2
