# Local changes against dfu-programmer v1.1.0

Everything below is mechanical: build plumbing or C++/MSVC conformance. No behaviour changes
to the DFU protocol, the chip tables or the timing. Re-apply these if the vendored copy is ever
updated from upstream.

## 1. Build configuration (new files)

| File | Purpose |
|---|---|
| `config.h` | Hand-written replacement for autoconf's `config.h`: `PACKAGE*`/`VERSION` = 1.1.0, `HAVE_LIBUSB_1_0`, `HAVE_MALLOC`, `HAVE_STDLIB_H`. Includes the two headers below. |
| `compat-msvc.h` | MSVC only: `strcasecmp`/`strncasecmp` → `_stricmp`/`_strnicmp`, `usleep()` → `Sleep()`, plus `<windows.h>`. |
| `dfu_redirect.h` | All platforms: `printf`/`fprintf`/`vfprintf` → `dfu_out_*()` (implemented in `../../dfuprogrammerqt.cpp`) so the tool's output reaches the dialog, and `isatty()` → 0 to switch off the `\r` progress meter. |

## 2. LibreSSL-style path and POSIX fixes

| File | Change |
|---|---|
| `dfu.cpp`, `dfu.h`, `dfu-device.h`, `libdfu.cpp` | `<libusb-1.0/libusb.h>` → `<libusb.h>` (the header is in the repo root, `INCLUDEPATH` has it) |
| `atmel.cpp` | removed `#include <unistd.h>` (does not exist on MSVC) |
| `atmel.cpp` | `atmel_select_memory_unit(device, …)` calls: added `(atmel_memory_unit_enum)` casts (3 sites) |
| `atmel.cpp` | `row->offset + (void *)info` → `(int16_t *)((char *)info + row->offset)` (`void*` arithmetic is a GNU extension) |

## 3. `const` correctness (required by MSVC conformance mode, `/permissive-` in Qt 6)

The values are only ever read, so this is a type change with no behaviour change.

| File | Change |
|---|---|
| `dfu.cpp` | `dfu_state_to_string()` / `dfu_status_to_string()` return `const char*`; local `char *message` → `const char *` |
| `dfu.h` | matching prototypes for the two functions above |
| `dfu.cpp` | `char *msg` → `const char *msg` in `dfu_msg_response_output()` |
| `commands.cpp` | `char *message` → `const char *message` (2 sites) |
| `atmel.cpp` | `char *mem_names[]` → `const char *mem_names[]` |

## 4. `config.h` include added to three translation units

`dfu.cpp`, `intel_hex.cpp`, `util.cpp` did not include `config.h` (autoconf passes it on the
command line), so they would miss the output redirect. A `#include "config.h"` was added at the
top of each.
