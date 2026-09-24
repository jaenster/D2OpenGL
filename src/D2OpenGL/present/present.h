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

// Draws the frame into the window and swaps; false when the stage is not open.
bool Present_SwapBuffers(HDC hDC);

// GetCursorPos as the game sees it.
BOOL Present_GetCursorPos(POINT *pPt);

#endif

#endif
