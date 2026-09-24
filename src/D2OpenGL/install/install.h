#ifndef D2OPENGL_INSTALL_H
#define D2OPENGL_INSTALL_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Overwrites host code or data, but only if it still holds the expected bytes. */
BOOL install_patch(DWORD va, const void *expect, const void *bytes, SIZE_T len);

/* 1.14d: makes -opengl select D2gfx mode 5 and puts the table in that mode's slot. */
BOOL install_114d(void *const *table);

#ifdef __cplusplus
}
#endif

#endif
