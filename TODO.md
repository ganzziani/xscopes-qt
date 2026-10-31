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
- [ ] Optional cleanup: `Bitmaps/gt.ico` and `GT-logo-256.png` are now unreferenced.
- [ ] Once nothing else is needed from it, delete the `xscopes-qt-mine` folder (user's call).

## 3. Qt5 → Qt6 port
About 100 compile errors with Qt 6.11.1 MSVC (local copy, test build 2026-10-31).
- [ ] QCustomPlot: replace with 2.1.1 (Qt6-ready, copy from `Xscopes-Qt6/source`) and adapt call sites.
      The alternative is patching 1.x.
- [ ] `main.cpp`: drop `QTextCodec`.
- [ ] `xprotolabinterface.cpp`: `endl` → `Qt::endl`, `QApplication::desktop()` → `QScreen`,
      `QColor::getHslF` takes `float*`, `QString::append` overload (line ~1529),
      `QMouseEvent::x()/y()` → `position()`.
- [ ] `libusbdevice.cpp`: 19 errors, mostly the `QtConcurrent::run` signature change.
- [ ] `customtheme.cpp`: 2 errors.
- [ ] Clear the warnings that matter (deprecated APIs).

## 4. Windows libraries
- [ ] Replace `libs/win/libusb-1.0.*`. The current files are 32-bit (i386) and won't link with a 64-bit kit.
      Use the VS2022 x64 build from a current libusb release.
- [ ] `bootloader.cpp` hard-codes `program = "D:/dfu-programmer"`. Ship `dfu-programmer.exe` next to the app instead.

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
