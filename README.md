xscopes-pc
==========

XScopes PC Interface — Qt desktop software for the Gabotronics XScopes / Xprotolab
oscilloscopes and logic analyzers, and for the Oscilloscope Watch. It plots the device's
acquisition over USB or a serial port, controls the instrument, and updates its firmware.

Copyright (C) Gabotronics. Licensed under the GNU GPL v3 or later, see [gpl.txt](gpl.txt).

History
-------

1.0.0.2 - Fixes for QT5
1.0.0.1 - QT4 version

Third-party software
--------------------

This application is built on the work of others. Thank you:

* **[dfu-programmer](https://github.com/dfu-programmer/dfu-programmer)** — firmware updates over
  the devices' ATxmega DFU bootloader. Written by **Weston Schmidt**, with many contributors since;
  the full list is in [third_party/dfu-programmer/AUTHORS](third_party/dfu-programmer/AUTHORS).
  Vendored at version 1.1.0 and linked into the application (GPL-2.0-or-later); local build
  changes are listed in [third_party/dfu-programmer/PATCHES.md](third_party/dfu-programmer/PATCHES.md).
* **[QCustomPlot](https://www.qcustomplot.com/)** — the plotting widget, version 2.1.1.
  Copyright (C) 2011-2022 **Emanuel Eichhammer** (GPL-3.0).
* **[QextSerialPort](https://github.com/qextserialport/qextserialport)** — serial port access, in
  `extserialport/`. Copyright (C) **Wayne Roth**, **Stefan Sander**, **Michal Policht**,
  **Brandon Fosdick**, **Liam Staskawicz** and **Debao Zhang** (MIT-style licence, see the file
  headers).
* **[Liberation Mono](https://github.com/liberationfonts/liberation-fonts)** — the bundled
  monospace font, version 2.1.5. Metric compatible with Courier New, Copyright (C) 2010 **Google
  Corporation** and 2012 **Red Hat, Inc.** (SIL OFL 1.1).
* **[libusb](https://libusb.info/)** — USB access, version 1.0.30. Copyright (C) **Johannes Erdfelt**,
  **Daniel Drake**, **Pete Batard**, **Nathan Hjelm** and **Chris Dickens** (LGPL-2.1).
