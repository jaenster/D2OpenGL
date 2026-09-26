#ifndef D2OPENGL_PRESENT_PRESENT_H
#define D2OPENGL_PRESENT_PRESENT_H

// The present stage: with scaling on, the renderer draws into an off-screen buffer of the game's size
// (640x480 or 800x600) and every swap draws that buffer into a larger window through a filter or the
// CRT shader. The game keeps believing its window is the game's size: its mouse messages and the
// cursor and window-geometry calls it imports are translated between the two.
//
// Settings come from D2OpenGL.ini next to the DLL; without it the defaults apply. With
// Scaling=off none of this runs and the renderer behaves exactly as without the present stage.

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reads the settings and, when scaling is on and -opengl is on the command line, makes the process
// DPI-aware and redirects the game modules' cursor and window-geometry imports. Must run before the
// game creates its window.
void present_install(HINSTANCE hSelf);

// Redirects the imports of game modules loaded since the last call.
void present_patch_modules(void);

#ifdef __cplusplus
}

// Fullscreen without a display mode switch (borderless): the renderer skips its mode changes.
bool Present_KeepsDisplayMode(void);

// Called with the new context current, after the window covers what it should. Sizes the window,
// creates the off-screen buffer and binds it; false when the stage is off or unavailable (the
// renderer then draws straight into the window).
bool Present_Open(HWND hWnd, HDC hDC, int nWidth, int nHeight, bool bFullscreen);

// Frees the stage's GL objects; the context must still be current.
void Present_Close(void);

// The factor the off-screen buffer is larger than the game in each direction: 1 when the stage is
// not open. The renderer's projection stays in game coordinates; its viewport covers the buffer.
int Present_RenderScale(void);

// Whether sprites are upscaled (Upscale=mmpx) when the render scale is above 1.
bool Present_UpscaleSprites(void);

// The HD pack file (HDPack, or D2OpenGL.hd next to the DLL), or NULL for none.
const char *Present_HDPackPath(void);

// The renderer's screen read (slot 10) wants a game-sized image: between these two calls the bound
// framebuffer holds the frame at the game size. Nothing happens at render scale 1.
void Present_BeginReadBack(void);
void Present_EndReadBack(void);

// Draws the frame into the window and swaps; false when the stage is not open.
bool Present_SwapBuffers(HDC hDC);

// ToggleKey was pressed: the next swap switches between upscaled and original sprites.
void Present_RequestToggle(void);

// GetCursorPos as the game sees it.
BOOL Present_GetCursorPos(POINT *pPt);

#endif

#endif
