# dfu-programmer 1.1.0 (vendored)

ATxmega DFU programmer, linked into the XScopes Interface so firmware upgrades work on every
platform the application is built for. Replaces the external `dfu-programmer.exe`.

- Upstream: <https://github.com/dfu-programmer/dfu-programmer>, release **v1.1.0** (2023-07-08)
- Files taken from `src/` of that tag, renamed `.c` → `.cpp` so qmake/CMake compile them as C++
  (MSVC compiles `.c` as C89, which several files do not obey)
- Only the library entry point is used: `dfu_programmer(struct programmer_arguments *)` from
  `libdfu.h`. `main.c` is not vendored — the application builds the argument list itself
  (`../../dfuprogrammerqt.cpp`) and runs it on a worker thread
- Devices are reached through libusb 1.0, the same library the application already links on
  Windows, Linux and macOS, so no extra runtime is needed

## Local changes

See [PATCHES.md](PATCHES.md). They are limited to: a hand-written `config.h` (no autoconf),
an MSVC compatibility header, routing the tool's stdout/stderr into the application
(`dfu_redirect.h`), and a handful of `const`/cast fixes that C++ requires.

Nothing in the DFU logic itself was changed: no protocol, timing or chip-table edits. The two
bugs that our older fork carried patches for are latent here and were reported upstream instead
(64 KB read clamp, missing `goto finally` on a page-select error).

## Credits

`dfu-programmer` was written by **Weston Schmidt** and has been improved over the years by a long
list of contributors — chip support, the AVR32 and XMEGA backends, libusb 1.0 support, the
`--ignore-outside`/`--validate-first`/`--erase-first` options and the Windows builds all came from
them. The full list, with what each person contributed, is in [AUTHORS](AUTHORS) (copied verbatim
from the upstream tag).

If you find this application useful for flashing Gabotronics devices, consider thanking the
dfu-programmer project too: <https://github.com/dfu-programmer/dfu-programmer>.

## License

GNU GPL v2 or later ([COPYING](COPYING)), which is compatible with the application's GPLv3.
Keep the copyright headers in every file.
