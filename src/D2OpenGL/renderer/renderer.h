#ifndef D2OPENGL_RENDERER_RENDERER_H
#define D2OPENGL_RENDERER_RENDERER_H

// Everything a recovered function needs: types, the texture classes, the globals, the game bindings,
// the prototypes of every renderer file and the halt idiom.

#include <string.h>

#include "types.h"
#include "textures.h"
#include "globals.h"
#include "../host/host.h"
#include "../platform/glext.h"
#include "../platform/platform.h"
#include "textures_fns.h"
#include "core.h"
#include "draw2d.h"
#include "sprites.h"
#include "tiles.h"

// The Mac halt idiom fp_fatal("Unrecoverable internal error %08x line %d", GetReturnAddress(), line),
// Windows ERROR_UnrecoverableInternalError_Halt("", GetInstructionPointer(), line) + _exit(-1).
// OGL_GetReturnAddress is never inlined, so like the Mac's GetReturnAddress it yields an address
// inside the halting function.
uintptr_t OGL_GetReturnAddress(void);
[[noreturn]] void OGL_Halt(uintptr_t nAddress, int nLine);
#define OGL_HALT(line) OGL_Halt(OGL_GetReturnAddress(), (line))

// Storm operator new / new[] / delete / delete[] (Mac 000095fc, 000096b0, 00009632, 00009671), which
// Windows inlines as SMemAlloc/SMemFree with STORM.CPP and these line numbers.
void *OGL_StormNew(size_t nSize);
void *OGL_StormNewArray(size_t nSize);
void OGL_StormDelete(void *p);
void OGL_StormDeleteArray(void *p);

#endif
