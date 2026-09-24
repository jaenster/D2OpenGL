#ifndef D2OPENGL_INSTALL_H
#define D2OPENGL_INSTALL_H

#include <windows.h>

#include "../../common/build.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Overwrites host code or data, but only if it still holds the expected bytes. */
BOOL install_patch(DWORD va, const void *expect, const void *bytes, SIZE_T len);

/* 1.14d: makes -opengl select D2gfx mode 5 and puts the table in that mode's slot. */
BOOL install_114d(void *const *table);

/* 1.13d, 1.13c, 1.10f: with -opengl on the command line, makes D2gfx load this DLL as its renderer in
 * mode 5. */
BOOL install_dllera(GameBuildId build, const char *self_path);

#ifdef __cplusplus
}
#endif

#endif
