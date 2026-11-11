# TODO

Goal: `xscopes-qt` builds and runs on Qt 6.12.0 MSVC 2022 x64 with all local work merged in.
See [CLAUDE.md](CLAUDE.md) for context. Only open work lives here; what is already done is in the git history.

**Status 2026-11-10:** branch `modernize` (not pushed, no PR) builds, runs, connects to the Xprotolab over USB,
plots at the device frame rate and takes display screenshots. Firmware upgrade is the next big piece.

**Open decisions:**
- DFU route: external `dfu-programmer` exe, or link the code in (1c)? Suggestion: ship 1.1.0 now, land in-app
  behind a flag, drop the exe once the bench test passes.
- dfu-programmer base: v1.1.0 release, or v1.1.0 plus the bounded-retry hunk from upstream PR #105?
- Report the two dfu-programmer bugs we fix locally (1b) upstream?
- Take the installer, bootloader icons and About box from `Xscopes-Qt6` now, or leave packaging to the rewrite?

## 1. Firmware upgrade / DFU
Reference copies: `xscopes-qt_dfu/dfu` is the abandoned linked-in attempt, `Xscopes-Qt6/source` has the installer
and the About box, `xscopes-qt-mine` has nothing left to merge (only `exe/dfu-drivers/` is still needed, see 2).

### 1b. Which dfu-programmer we ship
`libs/win/dfu-programmer.exe` is a local MinGW-w64 15.2.0 build (May 2026) of `xscopes-qt_dfu/dfu`: statically
linked libusb, `--version` says 0.7.2 (stale `config.h`) yet it supports 0.9.0 options. It is tracked in git with
no build script, so it is not reproducible. Upstream is at **v1.1.0** (2023-07-08, `master` dormant, open PR #105
bounds the device-reset/status-retry loops and moves CI to x64). The official `dfu-programmer-x64-1.1.0.zip` works
(reports 1.1.0, lists atxmega32a4u/256a3u) and ships `libusb-1.0.dll`.
- [ ] Replace the mislabeled binary with an official/reproducible one (1.1.0 x64 + `libusb-1.0.dll`), or drop it
      entirely if 1c lands.
- [ ] `--ignore-outside` needs >= 0.9.0; a stock 0.7.2 build would reject it.
- [ ] Keep our two local fixes as patches (upstream still has both): the 64 KB clamp
      `block_end = 64KB*(page+1)-1` in `atmel_read()`, and the missing `goto finally` when `atmel_select_page()`
      fails.

### 1c. In-app DFU (vendored v1.1.0) -- MSVC port verified 2026-11-10
The v1.1.0 sources compile clean with MSVC 2022 x64 (as C++), link against `libs/win/libusb-1.0.lib` and run.
- [ ] Vendor `third_party/dfu-programmer/` (v1.1.0 `.c` files kept as `.cpp`; GPLv2-or-later is compatible with
      our GPLv3, keep the headers, add `COPYING` + `PATCHES.md`).
- [ ] Hand-written `config.h`: `PACKAGE*`, `PACKAGE_URL`, `VERSION` = 1.1.0, `HAVE_LIBUSB_1_0`, `HAVE_MALLOC`,
      `HAVE_STDLIB_H`.
- [ ] `compat-msvc.h`: `windows.h`, `io.h`, `strcasecmp`/`strncasecmp` -> `_stricmp`/`_strnicmp`, `usleep()`,
      `isatty`.
- [ ] `<libusb-1.0/libusb.h>` -> `<libusb.h>` in `dfu.cpp`, `dfu-device.h`, `libdfu.cpp`.
- [ ] `atmel.cpp`: drop `<unistd.h>`, 3 enum casts (lines 154/658/1232), and
      `int16_t *ptr = (int16_t *)((char *)info + row->offset);` (line 308).
- [ ] Re-apply 1b's two fixes plus the `libusb_strerror()` diagnostics (needs a `libusb_error` cast).
- [ ] Route output: ~250 `fprintf`/`printf` sites. Macro-shim `fprintf`/`vfprintf`/`printf` in `compat-msvc.h`
      into one Qt-side sink that emits progress lines, so `read` (Intel HEX on stdout) works too. `isatty()`
      false keeps the `\r` progress meter off.
- [ ] Call `dfu_programmer(struct programmer_arguments *)` from `QtConcurrent::run`. It creates its own libusb
      context, so it coexists with `LibUsbDevice`; fill the args with `parse_arguments()`.
- [ ] Map `enum return_codes_enum` (`DEVICE_ACCESS_ERROR`, `FLASH_WRITE_ERROR`, `VALIDATION_ERROR_*`) to dialog
      messages instead of scraping text.
- [ ] Sequence: `0xBB` -> `closeDevice()` -> poll for `03EB:2FF4` -> DFU -> `launch` -> poll for `16D0:06F9` ->
      reconnect (`deviceDisconnected()` already handles the disappearance).
- [ ] Keep the QProcess path behind a flag until the in-app path is proven on the bench.

### 1d. One-click entry into the bootloader
The upgrade button now sends `0xBB` (`LibUsbDevice::requestBootloader()` from
`on_upgradeFirmware_clicked()`, USB only) and then disconnects, so the dialog finds the Atmel DFU device.
- [ ] Verify on hardware: the USB path of `0xBB` in the XScopes firmware had never been exercised (the Watch
      calls it with `usb = 0` from its own menu). If the transfer stalls, retry with `wLength = 0`.
- [ ] Firmware, outside this repo: `XScopes/Firmware` branch `fix/bb-usb-only` (ded495d) guards `0xBB` with
      `if(!usb) break;` (+4 bytes), so a stray byte on the serial port cannot park the device in DFU. Builds;
      needs flashing and a serial-port test.
- [ ] Windows needs a libusb-compatible driver for the DFU device either way: Atmel `libusb0` INF
      (`xscopes-qt-mine/exe/dfu-drivers`) or WinUSB via Zadig. `libusb-1.0.dll` 1.0.30 supports the libusb0,
      WinUSB, libusbK and UsbDk backends, so both DFU routes need the same driver.

## 2. Packaging / release
- [ ] Firmware upgrade test on the bench, then the serial-port connection.
- [ ] `windeployqt` plus `Xscopes-Qt6/installer.nsi` adapted to Qt 6.12 (`Qt6Core/Gui/Widgets/Network/Concurrent/
      SerialPort/PrintSupport`, `networkinformation`, `imageformats/qico`, `opengl32sw.dll`). Do not copy
      `Xscopes-Qt6/CMakeLists.txt`: its `qt6_add_resources(... ${RESOURCE})` is broken.
- [ ] Installer: add `dfu-drivers/`, `docs/xscopes-manual.pdf` (`Xscopes-Qt6/exe/docs`), `ReadMe.txt` and the GPLv3
      text. Today it ships only the exe, so a clean Windows cannot flash.
- [ ] Optional: About dialog with version/build/date, plus `build.h` + `build_inc.bat` from `Xscopes-Qt6/source`.
- [ ] Rewrite README (Qt6/MSVC kit, `libusb-1.0.dll`, DFU route, version history).
- [ ] Optional: make `CMakeLists.txt` build with Qt 6.

## 3. Plot / UI leftovers
- [ ] Visual check with a device: horizontal/vertical cursors, tracers, CH1/CH2/CHD zero markers, trigger icons.
- [ ] `hcursorA/B.png` are 32x32; 32x31 with a blank last row would put the cursor line exactly on the tip row.
- [ ] Window trigger: `setTriggerLevelPosition()` offsets the second icon by +7 px and the drag code mixes a
      0-255 inverted level with plot coordinates. Check in Window trigger mode.
- [ ] Tune the CRT decay retention (0.35..0.78) and the copy count (21) if the trail looks too short or long.
- [ ] Optional: widen the Options-tab info label or enable word wrap (long messages clip).
- [ ] Optional: render throttle in roll mode (device sends ~750 frames/s, the app draws ~120) and `reserve()` in
      `plotData()`.

## 4. Robustness / threading
- [ ] `chData`/`dataLength`/`dataAvailable` are written by the libusb callback and read by `plotData()` with no
      synchronization, and the next transfer is submitted immediately, so a frame can be read while the device
      writes it. Double-buffer with a swap in the transfer callback.
- [ ] Optional: auto-reconnect on replug with `libusb_hotplug_register_callback()` (the callbacks exist but are
      disabled with `&& 0`).
- [ ] Later: replace `extserialport/` with QtSerialPort (already a dependency, but it changes serial I/O and needs
      hardware).

## 5. AWG frequency: show and set what the device really plays
The firmware truncates the timer period (firmware CR-027, left as is), so the output is at or above the request:
+1.01 % at 44 kHz, and a 125.17375 kHz request plays 129.03 kHz (+3.08 %).
- [ ] Make `actualFreq` mirror `BuildWave()` (`Source/awg.c`). For a request `c` in 0.01 Hz: limit it byte by byte
      (byte 3 = 0, byte 2 <= `0xBE`, 0 becomes 1); `cycles` = 32, halved while `cycles > 1` and
      `(c >> 16) < 0x18 >> k`; prescaler 256 when `c < 256`, else 1;
      `PER = (12500000 * cycles) / (c * prescaler) - 1`, truncated; realized
      f = `cycles * 32 MHz / (256 * prescaler * (PER + 1))`. Both `on_doubleSpinBoxDesiredFreq_valueChanged()`
      call sites still use an older model (16000 Hz thresholds, no prescaler path, period clamped at 31).
- [ ] Optional: send the request that lands nearest the user's target (worst error drops from 3.08 % to ~0.5 %).
- [ ] Limit the control to what the device can play (125.17 kHz request, 129.03 kHz realized).

## 6. Screenshot leftovers
- [ ] Not tested on an Xprotolab (128x64) yet, only on the Watch (128x128).
- [ ] Serial port: there `'C'` is an XMODEM transfer; the button currently reports that it needs USB.
- [ ] A read takes tens of ms, so a screenshot can mix two frames. Freeze with `stopScope()` around it.

## 7. Portability
- [ ] `extra/linux/64-xscope.rules`: `MODE="066"` should be `0666` or, better, `TAG+="uaccess"`, and match on
      `ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="06f9"` instead of the manufacturer string.
- [ ] macOS: `libs/mac/libusb-1.0*.dylib` is x86_64 only; Apple Silicon needs an arm64/universal libusb.
- [ ] Android: open the device through Java `UsbManager` + `libusb_wrap_sys_device()`; no QtSerialPort, and no
      external exe, so in-app DFU (1c) is the portable route.
- [ ] Older firmware on Windows still needs Zadig; optionally bind WinUSB from the installer with libwdi.
- [ ] Outside this repo (ask first): re-run the firmware `-mrelax` timing paths (firmware OPT-001), flash the
      `fix/bb-usb-only` branch (see 1d), and test the Watch's `winusb-msos` branch on real hardware (this PC caches
      `osvc` for 16D0:06F9/0101 from the Xprotolab).

## 8. Housekeeping
- [ ] `persistenceitem.h` is untracked. Commit `2cb4a37` contains the code that includes it but not the file, so
      that commit does not build on a clean checkout.
- [ ] Regenerate `gt.icns` (still the 2014 design) and drop it from the `.qrc`: the `.pro`'s `ICON =` reads the file
      directly, so the embedded copy is dead weight.
- [ ] Delete `xscopes-qt-mine` once `exe/dfu-drivers/` is copied here.
- [ ] Decide whether to uninstall Qt 6.11.1.
- [ ] Optional: a "debug but optimized" kit (`CONFIG+=force_debug_info` + `/O2`); debug builds plot ~5x slower.

## Later
- [ ] Continue the `Xscopes-Qt6` rewrite.
