#ifndef D2OPENGL_PRESENT_INTERNAL_H
#define D2OPENGL_PRESENT_INTERNAL_H

// Shared between the present stage's files.

#include <windows.h>

enum PresentScalingMode { SCALING_OFF, SCALING_AUTO, SCALING_FIXED };
enum PresentFilter { FILTER_NEAREST, FILTER_SHARP, FILTER_LINEAR };
enum PresentShader { SHADER_NONE, SHADER_CRT };
enum PresentUpscale { UPSCALE_NONE, UPSCALE_MMPX };

struct PresentConfig {
    PresentScalingMode eScaling;
    int nScale;        // SCALING_FIXED only
    bool bBorderless;  // Fullscreen=borderless
    PresentFilter eFilter;
    PresentShader eShader;
    int nRenderScale;  // 1..4: the off-screen buffer is the game size times this
    PresentUpscale eUpscale;
    float fScanlines;  // [CRT] Scanlines, 0..1
    float fMask;       // [CRT] Mask, 0..1
    float fGlow;       // [CRT] Glow, 0..1
    char szHDPack[MAX_PATH];  // HDPack: the pack file, or empty for none
    int nToggleKey;           // ToggleKey: virtual key that switches upscaled and original sprites, 0 = none
};

// config.cpp
extern PresentConfig g_presentConfig;
void Present_LoadConfig(HINSTANCE hSelf);

// window.cpp: the game window and the mapping between its real client area and the game's.
struct PresentImage {
    int nX, nY;           // the image's top-left in the real client area
    int nWidth, nHeight;  // the image's size in real pixels
};

// Takes over the game window: subclass, size, cursor clip. False when it cannot.
bool PresentWindow_Attach(HWND hWnd, int nGameWidth, int nGameHeight, bool bFullscreen);
// The window has no scaled image any more (the stage failed on it).
void PresentWindow_Detach(void);
// Where the image goes in the real client area, and the client size.
void PresentWindow_GetImage(PresentImage *pImage, int *pnClientWidth, int *pnClientHeight);
bool PresentWindow_IsMapped(HWND hWnd);
int PresentWindow_GameWidth(void);
int PresentWindow_GameHeight(void);
// Real screen point -> the game's screen point (its client origin plus game coordinates), and back.
void PresentWindow_ScreenToGame(POINT *pPt);
void PresentWindow_GameToScreen(POINT *pPt);

// imports.cpp: the real user32 functions the hooks and the window code call.
struct PresentUser32 {
    BOOL(WINAPI *GetCursorPos)(LPPOINT);
    BOOL(WINAPI *SetCursorPos)(int, int);
    BOOL(WINAPI *GetClientRect)(HWND, LPRECT);
    BOOL(WINAPI *GetWindowRect)(HWND, LPRECT);
};
extern PresentUser32 g_user32;
void PresentImports_Init(void);

// shaders.cpp
extern const char g_szPresentVertexShader[];
extern const char g_szPresentSharpShader[];
extern const char g_szPresentCrtShader[];

#endif
