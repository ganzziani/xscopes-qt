# Packaging

Only the Windows installer exists. macOS and Linux are sketched at the end.

The version is written once, as `VERSION` in [`XprotolabInterface.pro`](../XprotolabInterface.pro).
qmake puts it in the executable's version resource, the code gets it as `APP_VERSION_STRING`,
and the installer build reads it for the installer's name and version.

## Windows installer

### Prerequisites

* Qt 6.8 or newer, MSVC 2022 64-bit kit. Default `C:\Qt\6.12.0\msvc2022_64`; set `QTDIR` for
  another one.
* Visual Studio 2022 with the C++ desktop tools (found through `vswhere`).
* [NSIS 3](https://nsis.sourceforge.io/Download), on `PATH`, in `C:\Program Files (x86)\NSIS`,
  or named by `MAKENSIS`.
* Python 3 (`py.exe` or `python.exe` on `PATH`, or `PYTHON`) and the documentation of the kit's
  Qt version (`C:\Qt\Docs\Qt-<version>`, added with the Qt Maintenance Tool; `QTDOCS` for
  another place). `qt-third-party.py` writes the payload's `licenses\Qt-third-party.txt` from
  it: the notices of the third-party code inside the Qt DLLs, always for the Qt that ships.

### Build

```
packaging\windows\build-installer.bat
```

It builds the release executable out of tree, stages it with `windeployqt`, checks the files
and packs `XScopes-Interface-<version>-win64-setup.exe` into `<build dir>\installer`. The build
dir defaults to `%TEMP%\xscopes-qt-build\win64-release` (`XSCOPE_BUILD_DIR` changes it).

| Option | Meaning |
|---|---|
| `/machine` | all users, into Program Files, needs admin rights (default) |
| `/user` | current user only, into `%LOCALAPPDATA%\Programs`, no admin rights |
| `/nobuild` | reuse the executable already built, only re-pack |
| `/clean` | delete the build dir first |
| `/out <dir>` | write the installer there |

The staged `<build dir>\stage\core` folder is a complete portable build: zip it and it runs
wherever it is unpacked.

### What it installs

* `xscope.exe`, the Qt runtime (no translations, no OpenGL/DirectX compilers), `libusb-1.0.dll`
  (a separate DLL, so it stays replaceable under the LGPL) and the Visual C++ runtime next to
  the executable, so no `vcredist` is needed.
* `ReadMe.txt` (the user's notes, [windows/ReadMe.txt](windows/ReadMe.txt)), `gpl.txt`,
  `licenses\` and the generated `licenses\Qt-third-party.txt`. See
  [licenses/README.md](../licenses/README.md); none of them is optional.
* Optional Start menu entries (the app, the online manual as a `.url`, the uninstaller) and a
  desktop shortcut. The manual itself is not shipped; `MANUAL_URL` in `installer.nsi` points to
  the PDF on gabotronics.com.
* No USB driver. The firmware's MS OS descriptors make Windows bind WinUSB by itself; older
  firmware needs Zadig. Atmel's `libusb0` DFU driver is not shipped: libusb 1.0 cannot use it.

It closes a running copy first, removes an older version from the same folder, keeps the
folder chosen last time, and writes the "Installed apps" entry and the `App Paths` key. The
uninstaller removes all of it and asks whether to delete the settings too.

NSIS was chosen over the Qt Installer Framework, which was tried on 2026-11-19: the QIF
installer was 3.6 times larger and every installation carries a 37 MB maintenance tool. QIF
is only worth it if all three platforms should share one installer framework.

### Code signing

Nothing is signed yet. Unsigned, the installer gets SmartScreen's "unrecognised app" warning,
and Windows Defender has flagged the NSIS uninstaller as `Trojan:Win32/Bearfoos.A!ml` (a
false positive, seen on 2026-11-19, withdrawn by Microsoft by 2026-11-22).

The build signs everything when a certificate is set: `SIGNCERT` (subject name) or `SIGNTHUMB`
(SHA-1 thumbprint), optionally `TIMESTAMP_URL` (default `http://timestamp.digicert.com`).

```
set SIGNCERT=Gabotronics S.A.
packaging\windows\build-installer.bat
```

The script signs `xscope.exe` with `signtool`, then NSIS signs the installer (`!finalize`) and
the uninstaller embedded in it (`!uninstfinalize`), through a generated, git-ignored
`windows\signing.nsh`.

Certificate options (checked 2026-11):

| Option | Cost | Notes |
|---|---|---|
| [SignPath Foundation](https://signpath.org) | free for open source | Signs as "SignPath Foundation", not Gabotronics. Needs the release built on GitHub Actions and approved per release. Signs only the finished installer, so the uninstaller stays unsigned. |
| Azure Artifact Signing | about $10/month | Organizations in the USA, Canada, EU and UK; individuals in the USA and Canada only. |
| OV certificate (DigiCert, Sectigo, ...) | $150-300/year | The key must be on a hardware token or a cloud HSM. |

Signing does not give SmartScreen reputation at once; it builds up with downloads under the
same publisher name, so keep that name stable.

To check a release for the Defender false positive, install it (a `/user` build needs no admin
rights) and scan the uninstaller:
`"%ProgramFiles%\Windows Defender\MpCmdRun.exe" -Scan -ScanType 3 -File <install dir>\uninstall.exe -DisableRemediation`
(exit code 2 means a detection). The uninstaller depends only on the script, the name and
version and the NSIS version, so one check covers a release. Report a detection at
<https://www.microsoft.com/en-us/wdsi/filesubmission>.

### Not done

* A certificate, see above.
* Binding WinUSB from the installer (libwdi), for PCs where Atmel's `libusb0` driver is
  already bound to the bootloader.
* ARM64 and 32-bit builds.

## macOS (planned)

1. `qmake && make` with a macOS Qt 6 kit gives `xscope.app`. First: an arm64 or universal
   `libs/mac/libusb-1.0.dylib` (the current one is x86_64 only) and a new `gt.icns`; set
   `QMAKE_TARGET_BUNDLE_PREFIX` for the bundle identifier.
2. `macdeployqt xscope.app -dmg`.
3. Sign and notarize (`codesign --options runtime`, `xcrun notarytool submit --wait`,
   `xcrun stapler staple`), or Gatekeeper blocks the download.

## Linux (planned)

All options need a working udev rule first: `extra/linux/64-xscope.rules` has `MODE="066"`,
matches on the manufacturer string and misses the DFU bootloader IDs.

* `make install`: add `target.path`/`INSTALLS`, a `.desktop` file and the udev rule to the `.pro`.
* Debian package: [`debian/`](../debian) is the old Qt 4/5 skeleton; it needs Qt 6 build
  dependencies and the udev rule.
* AppImage with `linuxdeploy` and its Qt plugin; it still needs the udev rule installed.
