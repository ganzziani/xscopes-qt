# TODO

Goal: `xscopes-qt` builds and runs on Qt 6.12.0 MSVC 2022 x64 with all local work merged in.
See [CLAUDE.md](CLAUDE.md) for context. Only open work lives here; what is already done is in the git history.

**Status 2026-11-12:** branch `modernize` (not pushed, no PR). Builds and runs on Qt 6.12.0 MSVC x64, connects over
USB (WinUSB, no Zadig), plots at the device frame rate, takes display screenshots, and updates firmware in-process
(dfu-programmer 1.1.0 linked in). Licensing is complete (`licenses/`). Both the Watch and the Xprotolab are usable.
The sniffer reads the Watch's 2569-byte payload (see 9) and was verified on the bench with real traffic. Channels'
AC coupling and derivative and the SPI sniffer's `/SS` invert are reachable from the app (2026-11-12), and the
cursors are switched from the plot's right-click menu (3).

**Open decisions:**
- Report the two dfu-programmer bugs we found (`atmel_read()` 64 KB clamp, missing `goto finally` on a page-select
  error) upstream?
- Take the About box and the NSIS installer skeleton from `Xscopes-Qt6` now, or leave packaging to the rewrite?

## 1. Firmware upgrade / DFU
Reference copies: `Xscopes-Qt6/source` has the installer skeleton and the About box, `xscopes-qt-mine` has nothing
left to merge (only `exe/dfu-drivers/` is still needed, see 2).

### 1a. Dialog and in-process programmer (done 2026-11-11/12)
`third_party/dfu-programmer/` is upstream **v1.1.0** (GPL-2.0-or-later, `PATCHES.md` lists the 6 mechanical
changes), wrapped by `dfuprogrammerqt.cpp`, which runs `dfu_programmer(&args)` on a worker thread and routes the
tool's output into the dialog. Verified on the Watch: `read --force` 256 KB and `read --eeprom` are **byte-identical
to the official 1.1.0 exe**, `erase` and `flash --eeprom` restore the EEPROM, and `launch` brings the app back.
The dialog itself was then driven end to end (Read Flash, Read EEPROM, then "Preserve EEPROM" Do Everything =
read EEPROM -> erase -> program flash -> program EEPROM -> launch) with the device image as its own input; every
step validated, the log carried the tool's messages, and the Watch came back on FW 2.97.
The external `dfu-programmer.exe`, the `QProcess` path and the `Dfu/UseExternalProgram` flag are gone.
Decided 2026-11-11: no `--force`. A manual "Program Flash" over non-blank flash keeps failing with "The target
memory for the program is not blank"; the button that erases first and does every step is the intended path.
Upstream is dormant; if it revives, PR #105 bounds the device-reset/status-retry loops and moves CI to x64.

### 1b. Windows driver for the DFU device
The Watch bootloader v1.3 and the XScopes bootloader answer MS OS 1.0 descriptors with compatible ID `WINUSB`, so
Windows 8+ binds WinUSB with no .inf and the app reaches the device through `libusb-1.0.dll`.
- [ ] Installer: legacy PCs still have Atmel's `atmel_usb_dfu.inf` (`libusb0`) bound, and a hardware-ID match
      outranks the compatible ID. Either document `pnputil /delete-driver oemNN.inf /uninstall /force`, or bind
      WinUSB from the installer with libwdi. `libusb0` *cannot* be used by libusb 1.0 without `libusbK.dll` in
      System32, which is not an option.
- [ ] Older firmware/bootloaders in the field have no MS OS descriptors: those units still need Zadig.

### 1c. One-click entry into the bootloader (settled 2026-11-11)
The upgrade button sends `0xBB` (`LibUsbDevice::requestBootloader()` from `on_upgradeFirmware_clicked()`, USB only)
and disconnects, then the dialog opens.
- **Watch: works.** Its bootloader (ASF, `03EB:2FEC`) stays in DFU after the application's software reset.
- **XScopes: not possible in software.** Atmel's FLIP bootloader (`bootloader.atxmega32a4u`, `03EB:2FE4`) decides at
  its reset vector (`cstartup.s90:136`): a software reset or a valid application sends it straight back, and the
  only other entry is the ISP pin (K1) at reset. Jumping past the decision into the boot section gives a device that
  enumerates only sometimes (measured 3/8 over a launch/`0xBB` cycle): its `sysclk_init_opt()` re-does the RC32M/DFLL
  setup while the application's USB-SOF-calibrated DFLL is still running, so the USB clock is often wrong. Tried and
  reverted: jump to the `BOOT_SECTION_START+0x1FC` trampoline, WDT off, USB teardown (`CTRLA`/`ADDR`), clock back to
  RC2M. All of it stayed flaky, so the experiment is gone from the firmware.
- The dialog now says so when the bootloader is missing: hold K1 and power-cycle, then press the button again.
- [ ] Long-term: the bootloader rewrite in `XScopes/Bootloader` (own USB protocol, MS OS descriptors, software
      entry) would make this a real one-click flow, but it needs a flashing client of its own — dfu-programmer does
      not speak that protocol. Decide before the rewrite work starts.
- [ ] Firmware, outside this repo: `XScopes/Firmware` master has ded495d, the `if(!usb) break;` guard for `0xBB`
      (+4 bytes), so a stray byte on the serial port cannot park the device in DFU. Flashed on the bench unit;
      still wants a serial-port test.

## 2. Packaging / release
- [ ] Serial-port connection test (never exercised on Qt 6; the USB path is covered).
- [ ] `windeployqt` plus `Xscopes-Qt6/installer.nsi` adapted to Qt 6.12 (`Qt6Core/Gui/Widgets/Network/Concurrent/
      SerialPort/PrintSupport`, `networkinformation`, `imageformats/qico`, `opengl32sw.dll`). Do not copy
      `Xscopes-Qt6/CMakeLists.txt`: its `qt6_add_resources(... ${RESOURCE})` is broken.
- [ ] Installer payload: `dfu-drivers/`, `docs/xscopes-manual.pdf` (`Xscopes-Qt6/exe/docs`), `ReadMe.txt` and the
      licence list in `licenses/README.md` (`gpl.txt`, `licenses/` incl. `OFL-1.1.txt`,
      `third_party/dfu-programmer/COPYING` + `AUTHORS`). Today it would ship only the exe, so a clean Windows
      cannot flash and the GPL/LGPL/OFL notices are missing. Keep `libusb-1.0.dll` replaceable (LGPL-2.1).
- [ ] Optional: About dialog with version/build/date, plus `build.h` + `build_inc.bat` from `Xscopes-Qt6/source`;
      it is also the natural place to repeat the credits at runtime.
- [ ] Finish README: the description and third-party credits are in, the build instructions (Qt6/MSVC kit,
      `windeployqt`, DFU route) and the version history are not.
- [ ] Optional: make `CMakeLists.txt` build with Qt 6.

## 3. Plot / UI leftovers
The cursors are switched from the plot's own right-click menu, and their state is `m_cursorChannel`,
`m_cursorVertical`, `m_cursorAuto` and `m_cursorTrack`: the Options tab's `groupBoxCursors` and the five widgets in
it are gone from the `.ui`. Checked on the Watch 2026-11-12: GPIO6 0x2C comes back as "CH1 / follow / vertical" in
the menu, and the four setters write their bits (CH2, automatic, follow, vertical in turn; follow and automatic
exclude each other).
Cursor heads are clamped inside the plot (`clampCursorY()`, the 8..504 margin the drag handler keeps) wherever
they are placed. The device reports each cursor relative to its channel's zero, so with CH2's position slider near
+127 cursor B landed at plot y 574 - outside 0..rangeMax - and was simply not drawn: CH2 showed cursor A only.
- [ ] Visual check with a device: tracers, CH1/CH2/CHD zero markers, trigger icons, and the horizontal cursor's
      "automatic" and "follow" placements.
- [ ] `hcursorA/B.png` are 32x32; 32x31 with a blank last row would put the cursor line exactly on the tip row.
- [ ] Window trigger: `setTriggerLevelPosition()` offsets the second icon by +7 px and the drag code mixes a
      0-255 inverted level with plot coordinates. Check in Window trigger mode.
- [ ] Tune the CRT decay retention (0.35..0.78) and the copy count (21) if the trail looks too short or long.
- [ ] Optional: widen the Options-tab info label or enable word wrap (long messages clip).

### 3a. Redraw pacing: the sluggishness at slow sample rates (done 2026-11-14)
The device streams its buffer at its own pace even when it acquired nothing new. Measured with libusb on the
Watch at 20ms/div and slower: ~57 transfers/s of the same 770-byte payload, with only 1..5 new frame numbers/s
(50s/div: 57 transfers/s, ~1 new frame/s). `plotData()` repainted every one of those transfers, and a slow-rate
repaint costs 3.6..5 ms (256 samples per channel plus the 8 CHD bits) against ~1 ms on a fast timebase, so the GUI
thread spent ~25 % of a core - and the debug build, ~5x slower, was saturated: that is the sluggishness. The app
cannot slow the device down (it is a push stream), so `plotData()` now repaints only when the payload changed -
`chData[768]` (frame counter) or `chData[769]` (write index, which roll mode and the progressive fill of a slow
sweep advance) - or when the UI asked for it via `m_repaint`, and device frames are capped at
`kMinRepaintIntervalMs` (30 ms; newer firmware streams ~750 frames/s in roll mode into a 128-pixel-wide plot).
Measured after the change: 1..2 repaints/s at 50s/div (the trace still grows one sample per repaint), ~30/s at
50ms/div, and 30/s at 8µs/div (was 57). Debug build at 50s/div: 7 % of a core instead of saturated.
The hidden `logging` that `readDeviceSettings()` switched on for 11+ (20ms/div and slower) is gone as well: it
appended every frame to `log.txt` (10 MB in 15 s, 6 MB in the old debug build's copy) and had no UI to turn it
off. `log.txt` still gets the settings dump when the app connects.
- [ ] Verify on an Xprotolab and in roll mode: the plot must still scroll/fill, and the persistence trail and the
      record button (both now age per payload with new samples rather than per USB transfer) must behave.
- [ ] Optional: `reserve()` the `plotData()` QVectors.

## 4. Robustness / threading
The flash of garbage on the first frame after launch is gone: `chData` is cleared when `LibUsbDevice` is created,
and `plotData()` only draws a whole USB frame (`dataLength == 770`). It used to draw whatever was in the buffer,
because the app sets `m_repaint` in `readDeviceSettings()` while the first transfer is still pending
(`dataLength` 0).
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

The Frequency Range buttons now reach the 0.01 Hz minimum the firmware plays: `radioButton0_1` is the
0.01..0.1 Hz decade (slider / 1000) and `radioButton1` the 0.1..1 Hz one (slider / 100); the desired-frequency
spin box accepts 0.01 Hz in 0.01 steps, and `readDeviceSettings()` selects both from the device's read-back (the
old `else` picked 100K for anything under 10 Hz). Verified on the Watch 2026-11-14: 0.01 Hz set in the app
survives an app restart (read-back 0.01), and 0.1 / 0.55 / 0.05 / 5 Hz round-trip through the UI. The requested
value is now rounded to 0.01 Hz (`qRound64`) and floored at `c = 1` instead of truncating, so the 0.01 Hz request
cannot become 0 and divide by zero.
- [ ] Make `actualFreq` mirror `BuildWave()` (`Source/awg.c`). For a request `c` in 0.01 Hz: limit it byte by byte
      (byte 3 = 0, byte 2 <= `0xBE`, 0 becomes 1); `cycles` = 32, halved while `cycles > 1` and
      `(c >> 16) < 0x18 >> k`; prescaler 256 when `c < 256`, else 1;
      `PER = (12500000 * cycles) / (c * prescaler) - 1`, truncated; realized
      f = `cycles * 32 MHz / (256 * prescaler * (PER + 1))`. `sendDesiredFrequency()` - shared by
      `on_doubleSpinBoxDesiredFreq_valueChanged()` and `editingFinished()` since 2026-11-14 - still uses an older
      model (16000 Hz thresholds, no prescaler path, period clamped at 31).
- [ ] Optional: send the request that lands nearest the user's target (worst error drops from 3.08 % to ~0.5 %).
- [ ] Limit the control to what the device can play (125.17 kHz request, 129.03 kHz realized).

## 6. Screenshot leftovers
- [ ] Not tested on an Xprotolab (128x64) yet, only on the Watch (128x128).
- [ ] Serial port: there `'C'` is an XMODEM transfer; the button currently reports that it needs USB.
- [ ] A read takes tens of ms, so a screenshot can mix two frames. Freeze with `stopScope()` around it.

## 7. Portability
- [ ] `extra/linux/64-xscope.rules`: `MODE="066"` should be `0666` or, better, `TAG+="uaccess"`, and match on
      `ATTRS{idVendor}=="16d0", ATTRS{idProduct}=="06f9"` instead of the manufacturer string. The same rule must
      also cover the DFU device (`03EB:2FEC` / `2FE4`) or a Linux user cannot flash.
- [ ] macOS: `libs/mac/libusb-1.0*.dylib` is x86_64 only; Apple Silicon needs an arm64/universal libusb.
- [ ] Android: open the device through Java `UsbManager` + `libusb_wrap_sys_device()`; no QtSerialPort. In-app DFU
      (1a) is the portable route there, but `dfu_device_init()` also opens devices by VID/PID and needs the same
      file-descriptor path.
- [ ] Outside this repo (ask first): re-run the firmware `-mrelax` timing paths (firmware OPT-001), flash the
      `fix/bb-usb-only` branch (see 1c), and test the Watch's `winusb-msos` branch on real hardware (this PC caches
      `osvc` for 16D0:06F9/0101 from the Xprotolab).

## 8. Housekeeping
- [ ] Regenerate `gt.icns` (still the 2014 design) and drop it from the `.qrc`: the `.pro`'s `ICON =` reads the file
      directly, so the embedded copy is dead weight.
- [ ] Delete `xscopes-qt-mine` once `exe/dfu-drivers/` is copied here.
- [ ] Decide whether to uninstall Qt 6.11.1.
- [ ] Optional: a "debug but optimized" kit (`CONFIG+=force_debug_info` + `/O2`); debug builds plot ~5x slower.

## 9. Sniffer
The geometry is now one place, `sniffer.h`: a `SnifferLayout` built by `snifferLayout(payloadLength)` (union =
payload - 8, RX half = (union - 1)/2, I2C = (union - 1)*4/5, `addr_ack` at `i2cSize`, indices little endian at
`unionSize`), byte-offset accessors instead of the `(Sniffer *)chData` cast, `LEN_BULK_IN_BUFFER` = 2570 and no
`>= 1289` guard: only a complete cycle (1289 XScopes, 2569 Watch) is a valid payload. A Watch read that starts
mid-cycle can end on exactly 1289 bytes (its parts 3+4), so `bcdDevice`'s device code (`LibUsbDevice::deviceCode`,
logged as "Device code") decides which of the two the device sends.

The sniffer settings byte (`sendSnifferSettings()`, `readDeviceSettings()`) now matches `Source/main.h`: bits 0-2
baud index, 3-4 UART data bits - 5 (the Data Bits spin box on the UART page), 5 parity enable / SPI `/SS` invert
(the Invert SS checkbox on the SPI page), 6 odd parity / SPI CPOL, 7 two stop bits / SPI CPHA, set with `|=`
because bit 7 legitimately carries two meanings and adding it twice wrapped. The parity read-back was swapped
(even/odd) and is fixed. Measured on the Watch: 0x18 -> clicking Invert SS gives 0x38 with the data bits intact.

Verified 2026-11-12 on the Watch: the settings byte round-trips through the app exactly for 8 device bytes covering
every field (0x00, 0x1C, 0x2E, 0x5C, 0x60, 0x9C, 0xE0, 0xF3 - the app reads the byte at connect, and the device
still has it afterwards). The display was then checked with the `Tester` fixture as the traffic source
(`Tester/Tools/app_sniffer.py`, 12/12 checks twice): UART RX and TX halves at 1280 bytes each for every data-bit
count 5..8, ASCII with nothing between the characters, I2C address/data/ack over 2048 bytes, SPI MOSI/MISO in
CPOL/CPHA mode 3, circular and linear modes. ~1000 cycles produced no rejected read and no bad frame; the only
dropped payloads were 244 scope frames arriving while the mode changed. A Watch read that starts mid-cycle only
ever ends on 649/1289/1929 bytes, which the layout and the device code reject.
- [ ] The XScopes path (1289-byte payload) is unverified on hardware since the bench DUT is the Watch now. The
      offsets it yields are the old 640/1024, but the longer transfer length and the device-code check are new.
- [ ] The UART decoder in the Watch loses a frame at 1200 baud in its longest format (8 data bits + parity + 2
      stop) when frames follow within a few ms; measured with the tester alone, so not this app. The bench harness
      leaves 25 ms between frames. Reported in `Tester/TODO.md` as W-13, for the firmware owner.

## Later
- [ ] Continue the `Xscopes-Qt6` rewrite.
