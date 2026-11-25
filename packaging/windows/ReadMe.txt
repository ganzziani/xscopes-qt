XScopes Interface
=================

PC software for the Gabotronics XScopes / Xprotolab oscilloscopes and the Oscilloscope Watch:
it shows the device's waveforms, controls the instrument and updates its firmware. This build
is for 64-bit Windows 10 and 11.

The device manual is online (also linked from the Start menu):
https://www.gabotronics.com/download/xscopes/xscopes-manual.pdf

Connecting
----------

The application connects to the device when it starts. If the device was plugged in later,
press "Connect" in the Options tab. No driver has to be installed: Windows sets up the
device by itself.

If it does not connect over USB, the firmware in the device is probably older and Windows
did not set it up. Install the WinUSB driver for it with Zadig (https://zadig.akeo.ie/),
then press "Connect" again.

To use a serial port instead, choose "Serial port" and the port in the Options tab.

Firmware updates
----------------

Press "UPGRADE FIRMWARE" in the Options tab; no other tool is needed. The XScopes/Xprotolab
bootloader starts only when you hold the K1 button while powering the device on; the
Oscilloscope Watch starts it on request. The update window tells you what to do.

If the bootloader cannot be found, set it up with Zadig (WinUSB) the same way. Atmel's
FLIP driver does not work with this application.

Uninstalling
------------

Use "Installed apps" in Windows Settings, or "Uninstall XScopes Interface" in the Start
menu. It asks whether to remove your settings as well.

If Windows Security reports "Trojan:Win32/Bearfoos.A!ml" for uninstall.exe, that is a false
positive on the unsigned uninstaller. Restore the file from Windows Security > Protection
history and run it, or delete the application folder by hand.

License and source code
-----------------------

Copyright (C) 2013-2026 Gabotronics. Free software under the GNU GPL v3 or later (gpl.txt),
with ABSOLUTELY NO WARRANTY. Third-party components and their licences: licenses\README.md

The complete source code of this version is published next to this installer, in the
release v@VERSION@ at https://github.com/ganzziani/xscopes-qt/releases/tag/v@VERSION@ :
XScopes-Interface-@VERSION@-source.zip (the application and the scripts that build this
installer), and the sources of the Qt @QTVER@ and libusb 1.0.30 libraries it ships with.
