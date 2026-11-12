/*
 * dfu-programmer writes its messages with fprintf(stderr, ...) and its Intel HEX
 * output with printf(). As a library inside a GUI that is useless: stderr goes
 * nowhere on Windows, and the HEX has to end up in a file, not in a console.
 *
 * config.h includes this header in every translation unit, so the tool's output
 * is routed to the sinks implemented in dfuprogrammerqt.cpp:
 *   - stdout  -> captured into a buffer (the "read" command's HEX output)
 *   - stderr  -> emitted as log lines that the bootloader dialog displays
 *
 * isatty() is forced to 0 as well, which switches off atmel.c's carriage-return
 * progress meter (it would fill the dialog with '>' characters). Everything the
 * user needs to see ("Reading 0x40000 bytes...", "Success", error texts) is
 * still printed, one line at a time.
 */
#ifndef DFU_PROGRAMMER_REDIRECT_H
#define DFU_PROGRAMMER_REDIRECT_H

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int dfu_out_printf(const char *format, ...);
int dfu_out_fprintf(FILE *stream, const char *format, ...);
int dfu_out_vfprintf(FILE *stream, const char *format, va_list args);

#ifdef __cplusplus
}
#endif

#define printf    dfu_out_printf
#define fprintf   dfu_out_fprintf
#define vfprintf  dfu_out_vfprintf

#define isatty(fd) 0

#endif /* DFU_PROGRAMMER_REDIRECT_H */
