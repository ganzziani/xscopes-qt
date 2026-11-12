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
| `XScopes\Firmware` | Device firmware (separate git repo, ATxmega32A4U, Atmel Studio 6.2, pinned AVR GCC 4.8.1, ~830 bytes of flash free). Its `AGENTS.md` covers building, sizing and flashing; `CODE_REVIEW_FINDINGS.md` lists open firmware issues. | separate repo |
| `Oscilloscope_Watch\Xproto-Watch-Firmware` | Oscilloscope Watch firmware (separate repo, ATxmega256A3U, Microchip Studio 7, avr-gcc 5.4.0). Same USB code and IDs as the XScopes. | separate repo |

Don't modify the sibling folders. Copy what's needed into this repo instead. Change the firmware repos only when
asked, on a branch, and leave their uncommitted work (e.g. probe settings in `Xprotolab.cproj`) alone.

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
- Runtime needs `libusb-1.0.dll` next to the exe. The `.pro` copies it from `libs/win` at build time (qmake
  `COPIES`). Run `windeployqt --release xscope.exe` for the Qt DLLs.
- Firmware upgrade runs dfu-programmer **inside** the app (`third_party/dfu-programmer`, v1.1.0, see
  `dfuprogrammerqt.cpp`); there is no external executable. Atmel DFU drivers (for PCs without the WinUSB
  descriptors) are in `xscopes-qt-mine/exe/dfu-drivers`.
- Needs Qt 6.8 or newer (`QStyleHints::setColorScheme`, used to keep the light palette the stylesheets expect).
- Debug builds: Qt asserts show as dialogs. To find the caller, run under `cdb` with breakpoints on
  `Qt6Cored!qt_assert` / `Qt6Cored!qt_assert_x`, or add a temporary `qWarning()`.

## Test hardware (this PC)

- An Xprotolab on USB (AWG wired to CH1), running firmware 2.97 with the MS OS descriptors: Windows binds
  `winusb.inf` by itself and the app connects. Older firmware needs Zadig (WinUSB) on Windows.
- The same board is on a JTAGICE3 (serial J30200015670, PDI) for flashing. See the firmware's `AGENTS.md`;
  flash with `erase -ap` + `program -fl` to keep the EEPROM calibration.
- An Oscilloscope Watch can take the same USB slot (it enumerates only in its oscilloscope mode, bcdDevice
  0x0120). Its stream is 2569 bytes per sniffer cycle, the XScopes' 1289 - `sniffer.h` holds both.
- The `Tester` jig (`D:\Projects\Tester`) sits between them: its PORTC bus is on the DUT's logic/sniffer pins, so
  it can play UART/I2C/SPI traffic for the app's Protocol Sniffer tab. `python Tools/app_sniffer.py` there drives
  the app through UI Automation and checks what it displayed (the app owns the DUT's USB, the jig's serial link
  does not conflict). Do not leave that jig's own suite running while the app is connected: one WinUSB handle.

## Code notes

- Main window: `xprotolabinterface.cpp/.h/.ui` (large). Device I/O: `libusbdevice.*`, `serialportconnection.*`.
  Firmware: `bootloader.*`, `dfuprogrammerqt.*`, `third_party/dfu-programmer/`, `filedownloader.*`.
  Theme: `customtheme.*`, `*.qss`.
- Plot markers (zero markers, cursor heads, trigger icons) are `MarkerPixmap` (`markerpixmap.h`): the pixmap's hotspot,
  not its top left corner, sits on the position. Give marker bitmaps an odd size so the hotspot is a center pixel.
- `qcustomplot.*` (QCustomPlot 2.1.1) and `extserialport/` (QextSerialPort, minimally patched for Qt6) are vendored
  third-party code. Patch them minimally, or replace them wholesale.
- Settings: the org/app name is set once in `main()`. Use a plain `QSettings settings;` everywhere, with theme keys under `Theme/`.
- Style: 4-space indent, opening brace on the same line, Qt signal/slot naming (`on_<widget>_<signal>`).
  Match the surrounding code.
- Git stores text with LF line endings (`core.autocrlf=true` locally).

## Icons (`Bitmaps/`)

- `GT-icon16/24/32/48/64/256.png`: the source images for the app icon.
- `GT-icon.ico`: all six sizes in one file. Used as the Windows exe icon (`xprotolabinterface.rc`) and as the window
  icon in `xprotolabinterface.ui` and `customtheme.ui`. Qt's QIcon loads every frame of an `.ico`, so each
  context gets the best size. Needs the `qico` image-format plugin at runtime (windeployqt includes it).
- `GT-icon24.png`: window icon of the bootloader dialog and the message box in `xprotolabinterface.cpp`.
- `GT-logo.png` (184x92): logo shown in the main window `.ui`.
- `gt.icns`: macOS bundle icon (`ICON` in the `.pro`). Outdated design, to be regenerated (see TODO.md).
- `LiberationMono-Regular.ttf` / `-Bold.ttf`: the monospace font the interface is laid out for, embedded in
  the `.qrc` and registered in the `XprotolabInterface` constructor, which keeps the family name it reports
  (`m_monospaceFamily`) for the meter readouts and plot labels. Metric compatible with Courier New, SIL OFL
  1.1 (`licenses/OFL-1.1.txt`). It replaced the bundled Courier New, whose EULA forbids redistribution.

## Workflow

- Commit in small steps on a feature branch. Don't push or open PRs without asking.
- Update TODO.md as items are done or discovered.
