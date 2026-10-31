# libusb for Windows (MSVC, x64)

- Version: libusb 1.0.30 (release 2026-05-17), from https://github.com/libusb/libusb/releases/tag/v1.0.30
- Files: `VS2022/MS64/dll/libusb-1.0.dll` and its import library `libusb-1.0.lib`
- Archive `libusb-1.0.30.7z` SHA-256 `7fb1dfec805b97983763d7d0ae244320da12add1003d4249c96cc4d586398c79`.
  The GPG signature was verified against Tormod Volden's key (C681 8737 9B23 DE9E FC46 651E 2C80 FF56 C683 0A0E).
- The matching header is `libusb.h` in the repo root.
- License: LGPL-2.1. It is dynamically linked, so `libusb-1.0.dll` must ship next to the exe.
