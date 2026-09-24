#ifndef D2OPENGL_PLATFORM_PLATFORM_H
#define D2OPENGL_PLATFORM_PLATFORM_H

// The Mac AGL/CoreGraphics/Carbon functions (Mac 002e0c38-002e1c3c, 002e70fc), rewritten on
// WGL/Win32. They keep the role and arity the Mac callers use; Mac handles become Windows ones
// (WindowRef -> g_hWnd, AGLContext -> g_hGLRC, CGDirectDisplayID -> HMONITOR). The game's HWND
// reaches g_hWnd through the slot 3/5/9 adapters (renderer/slots.cpp).
//
// The second half declares the platform services: the system calls the recovered renderer makes
// outside its own functions (Carbon, CoreGraphics, pthread, Apple GL extensions), on Win32.

#include "../renderer/types.h"
#include "glext.h"

// Mac 002e0c38: returns 1; on Windows also seeds the display options the Mac app layer sets
BOOL OGL_PlatformInitialize(HINSTANCE hInstance);

// Mac 002e0c42: 512 MB minus front and back buffer
int OGL_GetTextureMemoryBudget(void);

// Mac 002e0c87: Mac cursor callbacks; nothing on Windows
BOOL OGL_InstallCursorCallbacks(void);

// Mac 002e0cc8: 1999 D2OpenGL.dll OpenGLCloseSurface (ChangeDisplaySettings back)
BOOL OGL_RestoreDisplayMode(void);

// Mac 002e0d68: log OpenGLMacCreateSurface: fullscreen mode switch
BOOL OGL_PlatformCreateSurface(int nResolution);

// Mac 002e0f4a: log OpenGLMacSwapContext: mode switch on resize; 0 = OK
int16_t OGL_PlatformSwapContext(int nResolution, int nColorBits, int nUnused);

// Mac 002e10f6: log OpenGLMacOpenWindow: pixel format, context, viewport (1999 D2OpenGL.dll sOpenOpenGLWindow)
BOOL OGL_PlatformOpenWindow(int nResolution);

// Mac 002e15a6: log OpenGLMacSetRenderer: accept an accelerated renderer
BOOL OGL_PlatformSetRenderer(HDC hDC, int nPixelFormat);

// Mac 002e16cc: destroys the context (1999 D2OpenGL.dll OpenGLCloseSurface); never DestroyWindow the game's HWND
void OGL_DisposeWindow(void);

// Mac 002e173e: 1999 D2OpenGL.dll OpenGLBlit
void OGL_SwapBuffers(void);

// Mac 002e175d: Mac-only slot 11: not in the Windows table
int OGL_MacClampMouseEvent(void *pEvent);

// Mac 002e185d: Mac-only slot 12: not in the Windows table
BOOL OGL_MacGlobalToLocal(void *pPoint);

// Mac 002e1894: Mac-only slot 13: not in the Windows table
BOOL OGL_MacLocalToGlobal(void *pPoint);

// Mac 002e18cb: Mac slot 14 = Windows slot 11
BOOL OGL_ActivateWindow(BOOL bActive);

// Mac 002e18da: clamps to 55..255, then OGL_ApplyGamma
void OGL_SetGammaLevel(int nGamma);

// Mac 002e190b: fullscreen only
void OGL_ApplyGamma(void);

// Mac 002e194a: gamma (0, 100): fade to black before a mode switch
void OGL_GammaBlack(void);

// Mac 002e1985: gamma (100, 100) for cutscenes
void OGL_GammaIdentity(void);

// Mac 002e19c0: restores the display's own ramp
void OGL_RestoreGamma(void);

// Mac 002e19eb: fills the fullscreen display black
void OGL_ClearDisplay(void);

// Mac 002e1b4c: Blizzard's OpenGLMacStartCutscene
BOOL OGL_StartCutscene(BYTE **ppBuffer, int *pnPitch, int *pUnused);

// Mac 002e1bf5: frees the cutscene buffer
BOOL OGL_StopCutscene(void);

// Mac 002e1c3c OGL_SetOption: Mac slot 15 = Windows slot 12
BOOL OGL_SetOption(int nOption, int nValue);

// Mac 002e70fc: the display holding the window
HMONITOR OGL_FindDisplayDevice(HMONITOR hDisplay);

// ---- platform services

// Mac pthread_yield_np (OGL_EndCutScene): SwitchToThread()
void Platform_Yield(void);

// Mac Carbon FlushEvents (OGL_PlayBinkMovie): discard pending input messages of the Carbon mask classes (mDown 0x2, mUp 0x4, keyDown 0x8, keyUp 0x10, autoKey 0x20); never remove other messages
void Platform_FlushEvents(uint16_t wCarbonMask);

// Mac Carbon EventAvail (OGL_PlayBinkMovie): is an input message of the mask classes pending (PM_NOREMOVE)
BOOL Platform_EventAvail(uint16_t wCarbonMask);

// Mac CGEventCreate + CGEventGetLocation (OGL_PlayBinkMovie): GetCursorPos
void Platform_GetCursorPos(POINT *pPt);

// Mac LocalToGlobal({0,0}) (OGL_PlayBinkMovie): ClientToScreen(g_hWnd)
void Platform_GetClientOrigin(POINT *pPt);

// Mac 002f7324 (CG transfer table): SetDeviceGammaRamp with the Fog ramp builder g_host.BuildGammaRamp; save the original ramp once
void Platform_SetDisplayGamma(HMONITOR hDisplay, int nGamma, int nContrast);

// Mac 002f73f8: SetDeviceGammaRamp(saved ramp)
void Platform_RestoreDisplayGamma(HMONITOR hDisplay);

// Mac 00036ffe (best display mode search): EnumDisplaySettingsA loop; the closest mode at least
// nWidth x nHeight with nColorBits, returned when it is exact or bAcceptClosest is set
BOOL Platform_FindDisplayMode(HMONITOR hDisplay, int nWidth, int nHeight, int nColorBits, int nRefreshHz, BOOL bStretched, BOOL bAcceptClosest, DEVMODEA *pMode);

// Mac 0019f18a (>= 250 ms settle around context create/destroy): Sleep(250) or nothing
void Platform_SettleWait(void);

// Mac 002ffe21: the BinkCopyToBuffer surface type for the RGBA upload (Mac: 3 for 32-bit displays)
int Platform_GetBinkSurfaceType(HMONITOR hDisplay);
GLenum Platform_GetBinkPixelFormat(void);

// Mac glFinishObjectAPPLE(GL_TEXTURE, name) (COGLAGPTextures::AcquireSlot): glFinish()
void Platform_FinishTextureObject(GLuint nTexture);

// Mac atexit (OGL_InitPerspective): stores the function; platform_run_atexit runs the stored functions
// (last in, first out) when the DLL is unloaded by FreeLibrary, never at process exit
int platform_atexit(void (*pfnFunc)(void));
extern "C" void platform_run_atexit(void);

#endif
