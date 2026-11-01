# TODO

Goal: xscopes-qt builds and runs on Qt 6 (MSVC 2022 64-bit) with all local work merged in.
See [CLAUDE.md](CLAUDE.md) for context. Mark items `[x]` when done and add new ones as found.

## 0. Environment
- [x] Clone GitHub repo into `D:\Projects\xscopes-qt`. Old local copy renamed to `xscopes-qt-mine`.
- [x] Pick the kit: MSVC 2022 64-bit (not MinGW).
- [x] Update Qt: Qt Creator 20.0.2, Qt 6.12.0 MSVC 2022 64-bit + Serial Port (2026-10-31).
- [ ] After 6.12.0 builds, decide whether to uninstall Qt 6.11.1 (MSVC + MinGW).

## 1. Repo hygiene
- [x] Fix `.gitignore`: it had `*.cpp`, which ignored every new source file. It now also ignores
      `*.pro.user*`, `.qtcreator/` and build dirs.
- [x] Remove `Bitmaps/HcursorA.png` and `VcursorA.png`. They were byte-identical duplicates of the
      lowercase files the code uses, and the case-only name clash broke checkouts on Windows.
- [x] New 17x27 `vcursorA.png` / `vcursorB.png`. Odd width centers the pixmap on the cursor line.
      Cursor B now uses its own bitmap (renamed from `VcursorB.png`, added to `.qrc`), and the drag/position
      code derives offsets from the pixmap width instead of a hard-coded 32 px.
      Needs a visual check once the app builds.

## 2. Merge local work from `xscopes-qt-mine`
- [x] Imported as commit `46733a7` on branch `local-work` (based on `c537bf2`, dated 2023-12-11) and
      merged into `modernize` (`586dd91`). No textual conflicts. Upstream's `Gain` struct and mouse-position label are kept.
      Brought in: bootloader slot + `RequestInfo`, QSettings org/app name, QCustomPlot 1.3.2, GT-icon set,
      `nullptr`, `QDesktopWidget` removal.
- [x] Post-merge fix: restored `RC_FILE` (the local copy had dropped it). The `.rc` and the window icons in
      `xprotolabinterface.ui` / `customtheme.ui` now use `GT-icon.ico`. They used to point at `gt.ico`, which is no longer in the `.qrc`.
- [x] Not imported from `-mine`: `exe/` (dfu-programmer.exe and Atmel DFU drivers, see step 4) and `libs/unix/`
      (prebuilt libusb `.a`/`.so`, unused because the `.pro` links the system `-lusb-1.0`).
- [x] Removed `Bitmaps/GT-logo-256.png` (outdated, unreferenced).
- [x] Removed unused `Bitmaps/xscope.ico` and `xscope.png` (2013), plus the `.qrc` entry.
- [x] Removed `gt.ico`, superseded by `GT-icon.ico`. Kept the `.ico`'s 32 px frame and rewrote
      `GT-icon32.png` to match it, so the PNG sources and the `.ico` agree at every size.
- [ ] Update `gt.icns` (macOS): it still shows the outdated design (2014: 16/32/48/128 px + 256 px JPEG 2000).
      Regenerate it from the GT-icon PNGs. macOS wants 16/32/64/128/256/512 px plus @2x, so 128 and 512 px
      sources are needed. Also drop it from the `.qrc`: it's embedded (~125 KB per binary) but never loaded from
      there, because the `.pro`'s `ICON =` reads the file directly.
- [ ] Once nothing else is needed from it, delete the `xscopes-qt-mine` folder (user's call).

## 3. Qt5 → Qt6 port
Compiles cleanly with Qt 6.12.0 MSVC 2022 64-bit (2026-10-31). Linking waits on step 4 (64-bit libusb).
- [x] QCustomPlot 1.3.2 → 2.1.1 (copied from `Xscopes-Qt6/source`). Call sites adapted: no `addItem()`,
      `setPixelPoint`/`pixelPoint` → `setPixelPosition`/`pixelPosition`, `clearData()` → `data()->clear()`,
      `rpHint` → `rpRefreshHint`, fixed tick steps via shared `QCPAxisTickerFixed` on both axis pairs,
      plottable `setSelectable(QCP::stNone)`.
- [x] Qt6 API: drop `QTextCodec`, `Qt::endl`, `QScreen` instead of `QApplication::desktop()`, `getHslF(float*)`,
      explicit `QChar(byte)` appends, new `QtConcurrent::run(&fn, obj)` order, char compares instead of `QString(uint8_t)`.
- [x] QextSerialPort: `QRegExp` → `QRegularExpression`, `qSort` → `std::sort` (minimal patch to vendored code).
- [x] Unified QSettings: org/app name set once in `main()` before any widget is created (member dialogs read settings
      in their constructors). Theme lives in the `Theme/` group, and old "Gabotronics Xscope" settings are migrated once.
      This fixes the maximized state not being restored.
- [x] Warnings: `QMouseEvent::position()`, `activateWindow()`, CSV open failure now reported, `gains` table without `NULL`.
      Left alone: `Q_ENUMS` deprecations inside QCustomPlot 2.1.1, `QString::fromUtf16`/`QMetaType` in QextSerialPort,
      and unchecked `open()` of the built-in `.qss` resources.
- [x] 8x8 grid checked visually (2026-10-31).
- [x] Debug-build asserts (`!qIsNaN(value)`, then QCheckedInt "Overflow in operator+") in `QCPItemPixmap::getFinalRect`.
      `vCursorA/BHead` and `ch1/ch2ZeroHead` called `setPixelPosition()` while still in plot coordinates; converting
      through the zero-size axis rect during setup gave NaN. Now the type is set to `ptAbsolute` first. As a side effect,
      the CH1/CH2 zero markers now draw (they were invisible before).
- [x] Marker alignment: the custom QCustomPlot 1.1.1 patch `QCPItemPixmap::moveX/moveY` (dropped in the 1.3.2 upgrade)
      is replaced by `MarkerPixmap` (`markerpixmap.h`), a subclass whose hotspot sits on the position; QCustomPlot stays
      unmodified. Zero markers are now 18x15 (tip = middle row); each trigger icon has its own hotspot (stem column,
      dashed row). Checked with an Xprotolab: the CH1 zero marker tip sits on its line, the trigger icon on the level.
- [x] Lost `sizeChanged()` signal (another 1.1.1 patch): `on_sizeChanged()` now runs from an event filter on the plot.
- [x] Startup with a device asserted in debug: `ch1ZeroPos`/`ch2ZeroPos` were never initialized and
      `readDeviceSettings()` replots before setting them. Initialized in the constructor.
- [ ] `hcursorA/B.png` are 32x32; an odd height (e.g. 32x31, rows 29-31 are blank) would put the cursor line exactly
      on the tip row instead of half a pixel below.
- [ ] Window trigger: `setTriggerLevelPosition()` places the other window icon at `coordToPixel(level) + 7`, and the
      drag code mixes a 0-255 inverted level with plot coords. Check with a device in Window trigger mode.
- [ ] Visually check horizontal/vertical cursors and tracers with a device connected (zero markers and the
      rising-edge trigger icon are checked).
- [ ] Later: replace QextSerialPort with QtSerialPort (already a dependency; `serialportconnection.h` has a commented-out
      `QSerialPort *serial`). This drops the vendored library but changes serial I/O, so test with hardware.

## 4. Windows libraries
- [x] libusb 1.0.30, VS2022 x64 DLL + import lib, GPG-verified (see `libs/win/README.md`). Removed the 32-bit
      `libusb-1.0.dll.a`. **The app now links and runs on Qt 6.12.0** (smoke test 2026-10-31, no device attached).
- [x] `dfu-programmer` is looked up next to the app first, then on PATH. `libs/win/dfu-programmer.exe` (0.7.2, x64,
      self-contained) comes from `xscopes-qt-mine/exe`. The `.pro` copies it and `libusb-1.0.dll` into the build output.
- [x] Windows dark mode made the stylesheets unreadable (white text on light backgrounds). `main()` now pins
      `Qt::ColorScheme::Light`, which needs Qt 6.8 or newer.
- [ ] Consider updating dfu-programmer to 1.1.0. Check that its Windows build still works with the Atmel/WinUSB driver.
- [ ] Atmel DFU drivers (`xscopes-qt-mine/exe/dfu-drivers`) aren't in the repo yet. Decide in step 6 (installer) or step 4b.

## 4b. USB drivers and portability (decided 2026-10-31: keep libusb)
- [ ] Firmware (`D:\Projects\XScopes\Firmware`, separate git repo, outside this repo): add Microsoft OS 1.0
      descriptors so Windows 8+ binds WinUSB by itself, with no Zadig and no `.inf`. Reviewed 2026-10-31:
      - Today the device (16D0:06F9 rev 0100, vendor class, one interface) answers no MS OS request, and this PC runs it
        on WinUSB through a Zadig/libwdi `oem104.inf` ("libusbx.org", 2012).
      - Needed: string descriptor 0xEE "MSFT100" + vendor code, and the 40-byte Extended Compat ID "WINUSB" answered
        from the vendor request handler (`EVENT_USB_Device_ControlRequest` in `usb_xmega.c`) *before* `ProcessCommand()`,
        since every vendor request goes there. Vendor code `'M'` (not a command letter). Bump `ReleaseNumber` to 01.01
        so Windows re-reads the descriptors (it caches the answer per VID/PID/bcdDevice).
      - No Extended Properties / `DeviceInterfaceGUID` needed: libusb 1.0.30 opens a non-composite WinUSB device through
        its `GUID_DEVINTERFACE_USB_DEVICE` path (`windows_winusb.c`: "the first interface is the same as the device").
      - Cost, measured with the pinned AVR GCC 4.8.1 against the real objects: **+124 bytes** (Descriptors.o +80,
        usb_xmega.o +44) on every variant.
      - **Done 2026-11-01**, on firmware `master` (fast-forwarded from `winusb-msos`, not pushed): c9ecc95 adds the descriptors,
        f545f62 enables `-mrelax` (OPT-001), bfce6c4 makes it work: bcdUSB 2.00 (Windows skips the MS OS query
        for 1.10 devices), STALL for DEVICE_QUALIFIER and other unknown descriptors, and a literal bcdDevice 0x0101
        (`VERSION_BCD(01.01)` rounds to 0x0100 on AVR). All variants have 806..886 bytes free (Xprotolab 830).
      - **Verified on the Xprotolab:** with `oem104.inf` removed, Windows binds `winusb.inf` on its own
        (`USB\MS_COMP_WINUSB`, `osvc` = `01 4D`), and libusb 1.0.30 opens it and reads version 2.97. This app
        connects and runs normally on it (checked by Gabriel). Left: re-run the timing-sensitive firmware paths for
        `-mrelax`.
      - Testing on this PC: the existing `oem104.inf` matches the hardware ID and outranks `winusb.inf`, so it keeps
        winning after the update. Remove it first (admin: Device Manager → uninstall device + delete the driver, or
        `pnputil /delete-driver oem104.inf /uninstall`), replug, and expect "XScope" on WinUSB from `winusb.inf` and
        `HKLM\SYSTEM\CurrentControlSet\Control\usbflags\16D006F90101` value `osvc` = `01 4D`.
- [ ] Oscilloscope Watch firmware (`D:\Projects\Oscilloscope_Watch\Xproto-Watch-Firmware`, separate repo): same USB
      code and the same 16D0:06F9 IDs. The same change is committed there on branch `winusb-msos` (b2b5cb9, not merged
      or pushed, +142 bytes; it already linked with `-mrelax`). Builds; not flashed or tested on a Watch yet.
- [ ] Older firmware on Windows: until users upgrade, Zadig is still needed. Optionally have the installer bind WinUSB
      with libwdi (the library Zadig is built on).
- [ ] DFU bootloader mode is a separate USB device (Atmel FLIP, VID 03EB) and can't be changed on units in the field.
      It still needs a driver on Windows (libusb0 from `exe/dfu-drivers/atmel_usb_dfu.inf`, or WinUSB via Zadig/libwdi).
- [x] Replaced the bundled `libusb.h` with the 1.0.30 header. `libusb_set_debug()` → `libusb_set_option(LIBUSB_OPTION_LOG_LEVEL)`.
- [ ] Linux: `extra/linux/64-xscope.rules` has `MODE="066"`. That's probably meant to be `"0666"`; better still, use `TAG+="uaccess"`.
      Match on `ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="06f9"` instead of the manufacturer string.
- [ ] macOS: `libs/mac/libusb-1.0*.dylib` is x86_64 only. Apple Silicon needs an arm64/universal libusb.
- [ ] Android: open the device through Java `UsbManager` (permission prompt, file descriptor), then
      `libusb_wrap_sys_device()` with `LIBUSB_OPTION_NO_DEVICE_DISCOVERY`. `LibUsbDevice::openDevice()` needs a separate
      code path for this. QtSerialPort isn't available on Android.
- [ ] Firmware upgrade portability: running `dfu-programmer.exe` as a separate process won't work on Android.
      DFU code built into the app on top of libusb (the `xscopes-qt_dfu` idea) is the portable route. Weigh this in step 5.

## 5. Bring over from `xscopes-qt_dfu` (only what's worth it)
- [ ] Guard against an empty `dialog.selectedFiles()` in `bootloader.cpp`.
- [ ] Decide between linking dfu-programmer in (`dfu/`, unfinished) and keeping the external exe.
      `Xscopes-Qt6`'s working bootloader is the reference.

## 6. Verify and package
- [ ] Clean build, run, connect to a device over USB and over serial, then test a firmware upgrade.
- [ ] `windeployqt` deployment and an installer (see `Xscopes-Qt6/installer.nsi`).
- [ ] Update README (version history, build instructions).
- [ ] Optional: make `CMakeLists.txt` work with Qt6.

## Later
- [ ] Continue the `Xscopes-Qt6` rewrite.
