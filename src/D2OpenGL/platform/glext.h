#ifndef D2OPENGL_PLATFORM_GLEXT_H
#define D2OPENGL_PLATFORM_GLEXT_H

// The OpenGL names past GL 1.1 that the recovered code uses. opengl32.dll exports GL 1.1 only; the
// one newer entry point is glActiveTexture, reached through Platform_glActiveTexture.

#include <windows.h>
#include <GL/gl.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#endif
#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif
// Apple extensions: only the COGLAGPTextures path, never selected on Windows.
#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE
#define GL_UNPACK_CLIENT_STORAGE_APPLE 0x85B2
#endif
#ifndef GL_TEXTURE_STORAGE_HINT_APPLE
#define GL_TEXTURE_STORAGE_HINT_APPLE 0x85BC
#endif
#ifndef GL_STORAGE_CACHED_APPLE
#define GL_STORAGE_CACHED_APPLE 0x85BE
#endif

// Mac glActiveTexture (OGL_CutsceneBeginGL): wglGetProcAddress("glActiveTexture"); a no-op when absent.
void Platform_glActiveTexture(GLenum eTexture);

#endif
