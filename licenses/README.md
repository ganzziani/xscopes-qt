# Licences and third-party notices

The XScopes Interface is distributed under the GNU GPL v3 or later: the full text is
[`gpl.txt`](../gpl.txt) in the repository root, and it must ship next to the application binary.

It contains third-party code under other licences. Nothing here is optional: keep this folder,
`gpl.txt` and `third_party/dfu-programmer/COPYING` with any binary distribution.

| Component | Version | Licence | Licence text | Source |
|---|---|---|---|---|
| XScopes Interface (this application) | — | GPL-3.0-or-later | [`gpl.txt`](../gpl.txt) | this repository |
| [dfu-programmer](https://github.com/dfu-programmer/dfu-programmer) | 1.1.0, linked into the binary | GPL-2.0-or-later (compatible with, and distributed here under, GPLv3) | [`third_party/dfu-programmer/COPYING`](../third_party/dfu-programmer/COPYING) | [`third_party/dfu-programmer/`](../third_party/dfu-programmer/) — upstream tag `v1.1.0`, changes in `PATCHES.md`; credits in `AUTHORS` |
| [QCustomPlot](https://www.qcustomplot.com/) | 2.1.1 | GPL-3.0 | [`gpl.txt`](../gpl.txt) (same licence); notice in [`qcustomplot.h`](../qcustomplot.h) | [`qcustomplot.cpp`](../qcustomplot.cpp), [`qcustomplot.h`](../qcustomplot.h) — unmodified |
| [QextSerialPort](https://github.com/qextserialport/qextserialport) | vendored | MIT-style | [`MIT-QextSerialPort.txt`](MIT-QextSerialPort.txt) (also in every file header) | [`extserialport/`](../extserialport/) — Qt6 patch only |
| [libusb](https://libusb.info/) | 1.0.30, shipped as `libusb-1.0.dll` | LGPL-2.1 | [`LGPL-2.1.txt`](LGPL-2.1.txt) | <https://github.com/libusb/libusb/releases/tag/v1.0.30> (archive verified in [`libs/win/README.md`](../libs/win/README.md)) |
| [Liberation Mono](https://github.com/liberationfonts/liberation-fonts) | 2.1.5, embedded in the binary | SIL OFL 1.1 | [`OFL-1.1.txt`](OFL-1.1.txt) | [`Bitmaps/LiberationMono-Regular.ttf`](../Bitmaps/LiberationMono-Regular.ttf), [`-Bold.ttf`](../Bitmaps/LiberationMono-Bold.ttf) — copies of upstream 2.1.5, unmodified |

## What this means in practice

* **GPLv3 (application, QCustomPlot)**: the complete corresponding source is this repository.
  Any binary distribution must be accompanied by the GPLv3 text and access to that source.
* **GPL-2.0-or-later (dfu-programmer)**: its files are compiled into the application, so the
  combined binary is distributed under GPLv3 (permitted by the "or later" clause). Keep the
  copyright headers in the vendored files and ship `COPYING`. Credits: `AUTHORS`.
* **LGPL-2.1 (libusb)**: it is used as a shared library (`libusb-1.0.dll`), never statically
  linked, so a user can replace it with a modified libusb. Ship the licence text, this notice
  and, on request, the libusb source (link above). Do **not** link libusb statically.
* **MIT (QextSerialPort)**: the copyright and permission notice must accompany copies; the file
  headers do that, and `MIT-QextSerialPort.txt` exists for binary-only distributions.
* **SIL OFL 1.1 (Liberation Mono)**: the font may be bundled and modified, but it must ship with
  this licence, the copyright notice and the reserved font names, and it may not be sold on its
  own. It replaced Courier New, which was embedded until 2026-11-11 and whose EULA does not allow
  redistribution inside an application (see TODO "Fonts").

## Packaging checklist

When the installer is built (TODO item 2), its payload must include:

```
gpl.txt
licenses/README.md
licenses/LGPL-2.1.txt
licenses/MIT-QextSerialPort.txt
licenses/OFL-1.1.txt
third_party/dfu-programmer/COPYING
third_party/dfu-programmer/AUTHORS
```

and the installed `libusb-1.0.dll` must stay a drop-in replaceable DLL (do not merge it into the
executable).
