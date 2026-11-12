/*
 * Hand-written replacement for the config.h that dfu-programmer's autoconf
 * generates. dfu-programmer 1.1.0, vendored for the XScopes Interface:
 * see README.md for provenance and PATCHES.md for the local changes.
 */
#ifndef DFU_PROGRAMMER_CONFIG_H
#define DFU_PROGRAMMER_CONFIG_H

#define PACKAGE              "dfu-programmer"
#define PACKAGE_NAME         "dfu-programmer"
#define PACKAGE_TARNAME      "dfu-programmer"
#define PACKAGE_STRING       "dfu-programmer 1.1.0"
#define PACKAGE_VERSION      "1.1.0"
#define PACKAGE_BUGREPORT    ""
#define PACKAGE_URL          "https://github.com/dfu-programmer/dfu-programmer"
#define VERSION              "1.1.0"

/* We always talk to the device through libusb 1.0: the one the application already links. */
#define HAVE_LIBUSB_1_0      1
#define HAVE_MALLOC          1
#define HAVE_STDLIB_H        1

#if defined(_MSC_VER)
#include "compat-msvc.h"
#endif

/* Send the tool's output to the application instead of to stdout/stderr (all platforms). */
#include "dfu_redirect.h"

#endif /* DFU_PROGRAMMER_CONFIG_H */
