#ifndef D2OPENGL_RENDERER_TEXTURES_FNS_H
#define D2OPENGL_RENDERER_TEXTURES_FNS_H

// The texture manager (Mac 002b0e04-002b1718, 002de2d0-002de56a, 002e5f00-002e6fa4).
// Its 42 member functions are declared in textures.h; this is its one free function.

#include "types.h"
#include "textures.h"

// Mac 002e5f00: GL_APPLE_client_storage + GL_APPLE_texture_range + GL_EXT_texture_rectangle and > 256 MB RAM (g_host.pdwTotalPhysicalMemory)
bool OGL_CanUseAGPTextures(void);

#endif
