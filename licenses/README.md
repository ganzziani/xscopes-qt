# Licences and third-party notices

The XScopes Interface is distributed under the GNU GPL v3 or later: the full text is
[`gpl.txt`](../gpl.txt) in the repository root, and it must ship next to the application binary.

It contains third-party code under other licences. Nothing here is optional: keep this folder,
`gpl.txt` and `third_party/dfu-programmer/COPYING` with any binary distribution.

| Component | Version | Licence | Licence text | Source |
|---|---|---|---|---|
| XScopes Interface (this application) | — | GPL-3.0-or-later | [`gpl.txt`](../gpl.txt) | this repository |
| [Qt](https://www.qt.io/) 6 | 6.12.0, shipped as `Qt6*.dll` + plugins | LGPL-3.0, GPL-2.0 or GPL-3.0 (used here under GPL-3.0) | [`gpl.txt`](../gpl.txt) | <https://download.qt.io/archive/qt/6.12/6.12.0/submodules/> (`qtbase`, `qtsvg`, `qtserialport`) |
| Third-party code inside the Qt libraries (PCRE2, zlib, libpng, libjpeg-turbo, FreeType, HarfBuzz, …) | as bundled in Qt 6.12.0 | BSD, MIT, Zlib, IJG, FTL and others, per component | `Qt-third-party.txt`, written into each binary distribution (see below) | part of the Qt sources above |
| [dfu-programmer](https://github.com/dfu-programmer/dfu-programmer) | 1.1.0, linked into the binary | GPL-2.0-or-later (distributed here under GPL-3.0) | [`third_party/dfu-programmer/COPYING`](../third_party/dfu-programmer/COPYING) | [`third_party/dfu-programmer/`](../third_party/dfu-programmer/) — upstream tag `v1.1.0`, changes in `PATCHES.md`; credits in `AUTHORS` |
| [QCustomPlot](https://www.qcustomplot.com/) | 2.1.1 | GPL-3.0-or-later | [`gpl.txt`](../gpl.txt); notice in [`qcustomplot.h`](../qcustomplot.h) | [`qcustomplot.cpp`](../qcustomplot.cpp), [`qcustomplot.h`](../qcustomplot.h) — unmodified |
| [QextSerialPort](https://github.com/qextserialport/qextserialport) | vendored | MIT | [`MIT-QextSerialPort.txt`](MIT-QextSerialPort.txt) (also in every file header) | [`extserialport/`](../extserialport/) — patched for Qt 6 only |
| [libusb](https://libusb.info/) | 1.0.30, shipped as `libusb-1.0.dll` | LGPL-2.1-or-later | [`LGPL-2.1.txt`](LGPL-2.1.txt) | <https://github.com/libusb/libusb/releases/tag/v1.0.30> (archive verified in [`libs/win/README.md`](../libs/win/README.md)) |
| [Liberation Mono](https://github.com/liberationfonts/liberation-fonts) | 2.1.5, embedded in the binary | SIL OFL 1.1 | [`OFL-1.1.txt`](OFL-1.1.txt) | [`Bitmaps/LiberationMono-Regular.ttf`](../Bitmaps/LiberationMono-Regular.ttf), [`-Bold.ttf`](../Bitmaps/LiberationMono-Bold.ttf) — unmodified |
| Visual C++ runtime (`vcruntime140*.dll`, `msvcp140*.dll`, …) | from Visual Studio 2022 | Microsoft's redistribution terms for Visual Studio | — | not open source; redistributable as part of an application |

## What this means in practice

* **GPLv3 (application, QCustomPlot)**: a binary distribution must come with the GPLv3 text
  and the complete corresponding source. Under the GPLv3 that is more than this repository:
  it includes the shared libraries the application needs, so the Qt and libusb sources too.
* **Source with each release**: publish, next to every binary download, this repository's
  source for that release, the Qt source archives `qtbase-everywhere-src-6.12.0.tar.xz`,
  `qtsvg-everywhere-src-6.12.0.tar.xz` and `qtserialport-everywhere-src-6.12.0.tar.xz`, and
  `libusb-1.0.30.tar.bz2`. Offering them from the same place as the binary
  covers both the GPLv3 and the LGPL-2.1; a link to an upstream site alone does not, because
  that copy can disappear.
* **GPL-2.0-or-later (dfu-programmer)**: its files are compiled into the application, so the
  combined binary is distributed under GPLv3 (permitted by the "or later" clause). Keep the
  copyright headers in the vendored files and ship `COPYING`. Credits: `AUTHORS`.
* **LGPL-2.1-or-later (libusb)**: it is used as a shared library (`libusb-1.0.dll`), never
  statically linked, so a user can replace it with a modified libusb. Ship the licence text and
  this notice, and the libusb source as above. Do **not** link libusb statically.
* **Qt 6**: the modules used (Core, Gui, Widgets, Network, Svg, Serial Port) are available under
  the LGPLv3, the GPLv2 or the GPLv3. This application is GPLv3 anyway (QCustomPlot), so Qt is
  used under the GPLv3 and `gpl.txt` covers it. The DLLs ship unmodified, as `windeployqt`
  copies them.
* **Code bundled inside Qt**: Qt's DLLs contain third-party libraries whose licences (BSD, MIT,
  Zlib, IJG, FTL, …) require their copyright notices, and for some a credit, in the
  documentation of a binary distribution. `Qt-third-party.txt` carries them. It is not kept in
  the repository (the source does not contain Qt): the Windows installer build writes it into
  the payload's `licenses\` from the attribution pages in the documentation of the Qt version it builds with (`C:\Qt\Docs\Qt-<version>`,
  modules `qtcore`, `qtgui`, `qtnetwork`), leaving out components that Qt uses only on other
  platforms or to build itself. So it always matches the Qt DLLs it ships with.
* **MIT (QextSerialPort)**: the copyright and permission notice must accompany copies; the file
  headers do that, and `MIT-QextSerialPort.txt` exists for binary-only distributions.
* **SIL OFL 1.1 (Liberation Mono)**: the font may be bundled and modified, but it must ship with
  this licence, the copyright notice and the reserved font names, and it may not be sold on its
  own. It replaced Courier New, which was embedded until 2026-11-11 and whose EULA does not
  allow redistribution inside an application.

## Packaging checklist

Every binary distribution must carry these files. The Windows installer stages them from the
paths on the left and installs them as the paths on the right:

| In the repository | Next to the installed `xscope.exe` |
|---|---|
| `gpl.txt` | `gpl.txt` |
| `licenses/README.md` | `licenses/README.md` |
| generated by the installer build (see above) | `licenses/Qt-third-party.txt` |
| `licenses/LGPL-2.1.txt` | `licenses/LGPL-2.1.txt` |
| `licenses/MIT-QextSerialPort.txt` | `licenses/MIT-QextSerialPort.txt` |
| `licenses/OFL-1.1.txt` | `licenses/OFL-1.1.txt` |
| `third_party/dfu-programmer/COPYING` | `licenses/dfu-programmer/COPYING` |
| `third_party/dfu-programmer/AUTHORS` | `licenses/dfu-programmer/AUTHORS` |
| `third_party/dfu-programmer/PATCHES.md` | `licenses/dfu-programmer/PATCHES.md` |

The relative links in this file assume that layout (`../gpl.txt`), so keep `licenses/` and
`gpl.txt` side by side.

The installed `libusb-1.0.dll` must stay a drop-in replaceable DLL (do not merge it into the
executable); the Qt DLLs and plugin folders next to it are deployed unmodified by `windeployqt`.

And with every release, outside the installer: the source archives listed under "Source with
each release" above.
