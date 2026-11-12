/*
 * MSVC compatibility for dfu-programmer's POSIX-flavoured C.
 * Only pulled in by config.h when building with MSVC; on Linux, macOS and the
 * other Unix-like targets the original POSIX calls work unchanged.
 */
#ifndef DFU_PROGRAMMER_COMPAT_MSVC_H
#define DFU_PROGRAMMER_COMPAT_MSVC_H

#if defined(_MSC_VER)

#include <windows.h>
#include <string.h>

/* arguments.c compares command and target names case-insensitively. */
#define strcasecmp   _stricmp
#define strncasecmp  _strnicmp

/* atmel.c waits a tenth of a second between USB retries. */
#define usleep(us)   Sleep((DWORD)((us) / 1000))

#endif /* _MSC_VER */
#endif /* DFU_PROGRAMMER_COMPAT_MSVC_H */
