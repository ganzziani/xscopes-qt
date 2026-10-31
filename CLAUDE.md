# XScopes Interface (xscopes-qt)

Qt desktop app for Gabotronics XScopes / Xprotolab oscilloscopes and logic analyzers.
It talks to devices over USB (libusb) and serial (bundled `extserialport/` QextSerialPort),
plots with a vendored QCustomPlot, and upgrades firmware through the bootloader dialog.

Upstream: https://github.com/ganzziani/xscopes-qt (branch `master`). Work is tracked in
[TODO.md](TODO.md); read it first at the start of a session and keep it updated.

## Current goal

Bring this repo to a good, building, up-to-date state: merge in the unpushed local work,
port Qt5 → Qt6, and build with a 64-bit MSVC kit. The full rewrite (`Xscopes-Qt6`) comes later.

## Related folders (D:\Projects)

| Folder | What it is | Status |
|---|---|---|
| `xscopes-qt` | This repo, cloned from GitHub. The target for all changes. | active |
| `xscopes-qt-mine` | Gabriel's local copy. Forked from upstream `c537bf2` (2019-11-08), last edited Dec 2023, never pushed. Has the bootloader dialog, QCustomPlot 1.3.2, new icons and Qt6 prep. | temporary source to merge from, read-only |
| `xscopes-qt_dfu` | Sep 2023 attempt to compile dfu-programmer sources (`dfu/`) into the app. Unfinished. | source of ideas, read-only |
| `Xscopes-Qt6` | Rewrite (CMake, Qt6). Has a working bootloader that uses dfu-programmer, plus QCustomPlot 2.1.1. | reference, later work |

Don't modify the sibling folders. Copy what's needed into this repo instead.

## Build

- **Kit:** Desktop Qt 6.12.0 **MSVC 2022 64-bit**. Qt lives at `C:\Qt`. VS 2022 Community and Build Tools are installed.
  MinGW is not used. The `.pro` links libusb as an MSVC `.lib`.
- **Primary build file:** `XprotolabInterface.pro` (qmake). `CMakeLists.txt` is a Linux-oriented Qt5 leftover
  from contributors. Keep it working only if that's cheap.
- Required Qt modules: core, gui, widgets, printsupport, network, concurrent, serialport (Serial Port is an add-on).
- Command-line build (out of tree, from PowerShell):

  ```
  cmd /c "\"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat\" && cd /d <build-dir> && C:\Qt\6.12.0\msvc2022_64\bin\qmake.exe D:\Projects\xscopes-qt\XprotolabInterface.pro CONFIG+=release && nmake /NOLOGO release"
  ```

  Use a build dir outside the repo, such as the session scratchpad. Never build in the source tree.
- Runtime needs `libusb-1.0.dll` next to the exe (`windeployqt` covers the Qt DLLs).
- Firmware upgrade runs the external `dfu-programmer.exe`. Atmel DFU drivers are in `xscopes-qt-mine/exe/dfu-drivers`.

## Code notes

- Main window: `xprotolabinterface.cpp/.h/.ui` (large). Device I/O: `libusbdevice.*`, `serialportconnection.*`.
  Firmware: `bootloader.*`, `filedownloader.*`. Theme: `customtheme.*`, `*.qss`.
- `qcustomplot.*` and `extserialport/` are vendored third-party code. Patch them minimally, or replace them wholesale.
- Style: 4-space indent, opening brace on the same line, Qt signal/slot naming (`on_<widget>_<signal>`).
  Match the surrounding code.
- Git stores text with LF line endings (`core.autocrlf=true` locally).

## Workflow

- Commit in small steps on a feature branch. Don't push or open PRs without asking.
- Update TODO.md as items are done or discovered.
