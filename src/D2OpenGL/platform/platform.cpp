// The Mac AGL/CoreGraphics/Carbon functions on WGL/Win32, and the platform services the renderer
// calls. The window is the game's (g_hWnd, stored by the slot 3/5/9 adapters); this file never
// creates, destroys or subclasses it.

#include "../renderer/renderer.h"

#include "../../common/log.h"

// ---- display helpers (CGDirectDisplayID -> HMONITOR; NULL is the primary display)

// The GDI device name of a display, or NULL for the primary display.
static const char *Platform_DisplayDeviceName(HMONITOR hDisplay, MONITORINFOEXA *pInfo)
{
    if (hDisplay == NULL)
        return NULL;
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->cbSize = sizeof(*pInfo);
    if (!GetMonitorInfoA(hDisplay, reinterpret_cast<MONITORINFO *>(pInfo)))
        return NULL;
    return pInfo->szDevice;
}

// CGDisplayBounds: the display's rectangle on the desktop.
static void Platform_GetDisplayBounds(HMONITOR hDisplay, RECT *pRect)
{
    MONITORINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    if (hDisplay == NULL) {
        POINT ptOrigin = {0, 0};
        hDisplay = MonitorFromPoint(ptOrigin, MONITOR_DEFAULTTOPRIMARY);
    }
    if (GetMonitorInfoA(hDisplay, &info)) {
        *pRect = info.rcMonitor;
    } else {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = GetSystemMetrics(SM_CXSCREEN);
        pRect->bottom = GetSystemMetrics(SM_CYSCREEN);
    }
}

// A DC on the whole display (the gamma ramp belongs to the display, not to the window).
static HDC Platform_OpenDisplayDC(HMONITOR hDisplay)
{
    MONITORINFOEXA info;
    const char *szDevice = Platform_DisplayDeviceName(hDisplay, &info);
    if (szDevice == NULL)
        return GetDC(NULL);
    return CreateDCA(szDevice, NULL, NULL, NULL);
}

static void Platform_CloseDisplayDC(HMONITOR hDisplay, HDC hDC)
{
    if (hDC == NULL)
        return;
    MONITORINFOEXA info;
    if (Platform_DisplayDeviceName(hDisplay, &info) == NULL)
        ReleaseDC(NULL, hDC);
    else
        DeleteDC(hDC);
}

// CFEqual on two display modes.
static bool Platform_SameDisplayMode(const DEVMODEA &a, const DEVMODEA &b)
{
    return a.dmPelsWidth == b.dmPelsWidth && a.dmPelsHeight == b.dmPelsHeight &&
           a.dmBitsPerPel == b.dmBitsPerPel && a.dmDisplayFrequency == b.dmDisplayFrequency;
}

// CGDisplaySetDisplayMode: DISP_CHANGE_SUCCESSFUL (0) or the failure code.
static LONG Platform_SetDisplayMode(HMONITOR hDisplay, DEVMODEA *pMode)
{
    MONITORINFOEXA info;
    const char *szDevice = Platform_DisplayDeviceName(hDisplay, &info);
    pMode->dmSize = sizeof(*pMode);
    pMode->dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    return ChangeDisplaySettingsExA(szDevice, pMode, NULL, CDS_FULLSCREEN, NULL);
}

// CGDisplayCopyDisplayMode: the display's current mode.
static BOOL Platform_GetCurrentDisplayMode(HMONITOR hDisplay, DEVMODEA *pMode)
{
    MONITORINFOEXA info;
    const char *szDevice = Platform_DisplayDeviceName(hDisplay, &info);
    memset(pMode, 0, sizeof(*pMode));
    pMode->dmSize = sizeof(*pMode);
    return EnumDisplaySettingsA(szDevice, ENUM_CURRENT_SETTINGS, pMode);
}

// The Fixed 16.16 refresh of a mode record in Hz, as the Mac logs it.
static double OGL_DisplayModeRefreshAsDouble(const OGLDisplayModeRec &rec)
{
    return (double)(float)(double)rec.fxRefresh * 1.52587890625e-05;
}

// ---- the Mac functions

// Mac 002e0c38 OGL_PlatformInitialize
BOOL OGL_PlatformInitialize(HINSTANCE hInstance)
{
    (void)hInstance;
    // Windows D2gfx never sends SetOption 0-5: seed what the Mac app layer sets there.
    g_nColorBits = 32;
    g_aDisplayModes[0].dwSizeFlags = (480u << 16) | (640u << 1);
    g_aDisplayModes[0].fxRefresh = 0;
    g_aDisplayModes[1].dwSizeFlags = (600u << 16) | (800u << 1);
    g_aDisplayModes[1].fxRefresh = 0;
    g_aDisplayModes[2] = g_aDisplayModes[1];
    // Fullscreen switches the display to the game's own resolution. A DPI-unaware process would then be
    // handed a scaled-down window and GL drawable (400x300 for 800x600 at 200%), so the display mode's
    // pixels must be the window's. D2gfx creates the window after this call. Windowed stays DPI-unaware,
    // so Windows scales the 800x600 window as it does for the other renderers.
    if (g_bFullscreen == 1)
        SetProcessDPIAware();
    return TRUE;
}

// Mac 002e0c42 OGL_GetTextureMemoryBudget
int OGL_GetTextureMemoryBudget(void)
{
    uint32_t nFrameBuffer = 0x96000;
    if (g_nResolution == 1)
        nFrameBuffer = 960000;
    return (int)((uint32_t)g_bAcceleratedRenderer * 0x20000000u -
                 ((uint32_t)(g_nColorBits == 32) * 2 + 2) * nFrameBuffer);
}

// Mac 002e0c87 OGL_InstallCursorCallbacks
BOOL OGL_InstallCursorCallbacks(void)
{
    return TRUE;
}

// Mac 002e0cc8 OGL_RestoreDisplayMode
BOOL OGL_RestoreDisplayMode(void)
{
    if (g_bFullscreen == 1) {
        if (g_bDisplayModeChanged) {
            MONITORINFOEXA info;
            const char *szDevice = Platform_DisplayDeviceName(g_hDisplay, &info);
            DEVMODEA dmSaved;
            memset(&dmSaved, 0, sizeof(dmSaved));
            dmSaved.dmSize = sizeof(dmSaved);
            EnumDisplaySettingsA(szDevice, ENUM_REGISTRY_SETTINGS, &dmSaved);
            ChangeDisplaySettingsExA(szDevice, NULL, NULL, 0, NULL);
            g_host.LogWrite("OpenGLMacCreateSurface: *** ChangeDisplaySettings (%dx%d)",
                            (int)dmSaved.dmPelsWidth, (int)dmSaved.dmPelsHeight);
            g_bDisplayModeChanged = FALSE;
        }
    }
    return TRUE;
}

// Mac 002e0d68 OGL_PlatformCreateSurface
BOOL OGL_PlatformCreateSurface(int nResolution)
{
    if (g_bFullscreen != 1)
        return TRUE;

    DEVMODEA dmCurrent;
    g_bDisplayModeChanged = Platform_GetCurrentDisplayMode(g_hDisplay, &dmCurrent);
    if (!g_bDisplayModeChanged) {
        g_host.LogWrite("OpenGLMacCreateSurface: *** EnumDisplaySettings failed.");
        return FALSE;
    }

    const OGLDisplayModeRec &rec = g_aDisplayModes[nResolution];
    DEVMODEA dmFound;
    if (!Platform_FindDisplayMode(g_hDisplay, OGLDisplayMode_Width(rec), OGLDisplayMode_Height(rec),
                                  g_nColorBits, OGLDisplayMode_RefreshHz(rec),
                                  OGLDisplayMode_IsStretched(rec), TRUE, &dmFound)) {
        g_host.LogWrite("OpenGLMacCreateSurface: *** Couldn't find mode for %dx%dx%d %fHz%s.",
                        OGLDisplayMode_Width(rec), OGLDisplayMode_Height(rec), g_nColorBits,
                        OGL_DisplayModeRefreshAsDouble(rec),
                        OGLDisplayMode_IsStretched(rec) ? " (stretched)" : "");
        return FALSE;
    }

    if (Platform_SameDisplayMode(dmFound, dmCurrent)) {
        g_bDisplayModeChanged = FALSE;
        return TRUE;
    }

    LONG nResult = Platform_SetDisplayMode(g_hDisplay, &dmFound);
    g_host.LogWrite("OpenGLMacCreateSurface: *** ChangeDisplaySettings (%dx%d)", (int)dmFound.dmPelsWidth,
                    (int)dmFound.dmPelsHeight);
    if (nResult == DISP_CHANGE_SUCCESSFUL)
        return TRUE;
    g_host.LogWrite("OpenGLMacCreateSurface: *** ChangeDisplaySettings failed (%d)", (int)nResult);
    return FALSE;
}

// Mac 002e0f4a OGL_PlatformSwapContext
int16_t OGL_PlatformSwapContext(int nResolution, int nColorBits, int nUnused)
{
    (void)nUnused;
    if (nColorBits == 0)
        nColorBits = g_nColorBits;
    int16_t nResult = 0;
    if (g_bFullscreen == 1) {
        const OGLDisplayModeRec &rec = g_aDisplayModes[nResolution];
        DEVMODEA dmFound;
        if (!Platform_FindDisplayMode(g_hDisplay, OGLDisplayMode_Width(rec), OGLDisplayMode_Height(rec),
                                      nColorBits, OGLDisplayMode_RefreshHz(rec),
                                      OGLDisplayMode_IsStretched(rec), TRUE, &dmFound)) {
            g_host.LogWrite("OpenGLMacSwapContext: *** Couldn't find resolution %dx%dx%d %fHz%s.",
                            OGLDisplayMode_Width(rec), OGLDisplayMode_Height(rec), nColorBits,
                            OGL_DisplayModeRefreshAsDouble(rec),
                            OGLDisplayMode_IsStretched(rec) ? " (stretched)" : "");
            nResult = -50;
        } else {
            DEVMODEA dmCurrent;
            BOOL bHaveCurrent = Platform_GetCurrentDisplayMode(g_hDisplay, &dmCurrent);
            LONG nError = 0;
            if (!bHaveCurrent || !Platform_SameDisplayMode(dmCurrent, dmFound)) {
                nError = Platform_SetDisplayMode(g_hDisplay, &dmFound);
                g_host.LogWrite("OpenGLMacCreateSurface: *** ChangeDisplaySettings (%dx%d)",
                                (int)dmFound.dmPelsWidth, (int)dmFound.dmPelsHeight);
            }
            nResult = 0;
            if (nError != 0) {
                g_host.LogWrite("OpenGLMacSwapContext: *** ChangeDisplaySettings failed (%d)", (int)nError);
                nResult = (int16_t)nError;
            }
        }
    }
    return nResult;
}

// Mac 002e10f6 OGL_PlatformOpenWindow
BOOL OGL_PlatformOpenWindow(int nResolution)
{
    // The Mac creates, shows and activates its own window here when windowed; Windows draws into the
    // game's window, in both modes, through its DC.
    const char *szError;
    DWORD dwError = 0;
    g_hDC = GetDC(g_hWnd);
    if (g_hDC == NULL) {
        dwError = GetLastError();
        szError = "OpenGLMacOpenWindow: *** GetDC failed. (%lu)";
        goto fail;
    }

    {
        // AGL_PIXEL_SIZE g_nColorBits, AGL_RGBA, AGL_DOUBLEBUFFER; AGL_ACCELERATED is checked by
        // OGL_PlatformSetRenderer, AGL_RENDERER_ID has no WGL meaning.
        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = (BYTE)g_nColorBits;

        int nPixelFormat;
        for (;;) {
            HMONITOR hDevice = OGL_FindDisplayDevice(g_hDisplay);
            (void)hDevice;
            SetLastError(0);
            nPixelFormat = ChoosePixelFormat(g_hDC, &pfd);
            if (nPixelFormat != 0 || g_nRendererId == -1)
                break;
            g_nRendererId = -1;
            g_host.LogWrite("OpenGLMacOpenWindow: *** Requested engine not found. Will try any available engine.");
        }
        if (nPixelFormat == 0) {
            dwError = GetLastError();
            szError = "OpenGLMacOpenWindow: *** ChoosePixelFormat failed. (%lu)";
            goto fail;
        }
        // A window takes one pixel format for its lifetime: a reopen finds it already set.
        if (GetPixelFormat(g_hDC) != nPixelFormat && !SetPixelFormat(g_hDC, nPixelFormat, &pfd)) {
            dwError = GetLastError();
            if (dwError != 0) {
                szError = "OpenGLMacOpenWindow: *** SetPixelFormat failed. (%lu)";
                goto fail;
            }
        }

        g_hGLRC = wglCreateContext(g_hDC);
        if (g_hGLRC == NULL) {
            dwError = GetLastError();
            szError = "OpenGLMacOpenWindow: *** wglCreateContext failed. (%lu)";
            goto fail;
        }
        if (g_bFullscreen == 1) {
            // aglSetFullScreen: the drawable covers the whole display.
            RECT rcDisplay;
            Platform_GetDisplayBounds(g_hDisplay, &rcDisplay);
            if (!MoveWindow(g_hWnd, rcDisplay.left, rcDisplay.top, rcDisplay.right - rcDisplay.left,
                            rcDisplay.bottom - rcDisplay.top, TRUE)) {
                dwError = GetLastError();
                szError = "OpenGLMacOpenWindow: *** MoveWindow failed. (%lu)";
                goto fail;
            }
        }
        Platform_SettleWait();
        if (!wglMakeCurrent(g_hDC, g_hGLRC)) {
            dwError = GetLastError();
            szError = "OpenGLMacOpenWindow: *** wglMakeCurrent failed. (%lu)";
            goto fail;
        }

        if (g_bFullscreen == 1) {
            RECT rcDisplay;
            Platform_GetDisplayBounds(g_hDisplay, &rcDisplay);
            g_ptWindowOrigin.x = (short)rcDisplay.left;
            g_ptWindowOrigin.y = (short)rcDisplay.top;
            const OGLDisplayModeRec &rec = g_aDisplayModes[nResolution];
            glViewport((OGLDisplayMode_Width(rec) - g_nScreenWidth) / 2,
                       (OGLDisplayMode_Height(rec) - g_nScreenHeight) / 2, g_nScreenWidth, g_nScreenHeight);
        } else if (g_bFullscreen == 0) {
            POINT ptOrigin = {0, 0};
            ClientToScreen(g_hWnd, &ptOrigin);
            g_ptWindowOrigin = ptOrigin;
            glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
        }

        if (!OGL_PlatformSetRenderer(g_hDC, nPixelFormat)) {
            szError = "OpenGLMacOpenWindow: *** OpenGLMacSetRenderer failed.";
            goto fail;
        }
        g_nResolution = nResolution;
        return TRUE;
    }

fail:
    g_host.LogWrite(szError, dwError);
    OGL_DisposeWindow();
    return FALSE;
}

// The pixel format is drawn by a hardware driver (ICD, or an accelerated generic MCD).
static bool Platform_IsAcceleratedFormat(const PIXELFORMATDESCRIPTOR &pfd)
{
    return !(pfd.dwFlags & PFD_GENERIC_FORMAT) || (pfd.dwFlags & PFD_GENERIC_ACCELERATED);
}

// Mac 002e15a6 OGL_PlatformSetRenderer
BOOL OGL_PlatformSetRenderer(HDC hDC, int nPixelFormat)
{
    const char *szRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *szVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    d2log("platform: GL_RENDERER %s, GL_VERSION %s", szRenderer ? szRenderer : "(null)",
          szVersion ? szVersion : "(null)");

    PIXELFORMATDESCRIPTOR pfd;
    int nFormats = DescribePixelFormat(hDC, nPixelFormat, sizeof(pfd), &pfd);
    if (nFormats == 0) {
        g_host.LogWrite("OpenGLMacSetRenderer: *** DescribePixelFormat failed.");
        return FALSE;
    }
    // The renderer of the chosen format, then any accelerated renderer on the display.
    bool bFound = Platform_IsAcceleratedFormat(pfd);
    for (int i = 1; !bFound && i <= nFormats; i++) {
        PIXELFORMATDESCRIPTOR pfdAny;
        if (!DescribePixelFormat(hDC, i, sizeof(pfdAny), &pfdAny)) {
            g_host.LogWrite("OpenGLMacSetRenderer: *** DescribePixelFormat failed.");
            return FALSE;
        }
        if ((pfdAny.dwFlags & PFD_SUPPORT_OPENGL) && Platform_IsAcceleratedFormat(pfdAny))
            bFound = true;
    }
    if (!bFound) {
        g_host.LogWrite("OpenGLMacSetRenderer: *** No usable OpenGL engine found.");
        return FALSE;
    }
    g_bAcceleratedRenderer = true;
    return TRUE;
}

// Mac 002e16cc OGL_DisposeWindow
void OGL_DisposeWindow(void)
{
    if (g_hGLRC != NULL) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_hGLRC);
        g_hGLRC = NULL;
    }
    // The Mac disposes its own window here; the game's window stays, only its DC is released.
    if (g_hDC != NULL) {
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = NULL;
    }
    Platform_SettleWait();
}

// Mac 002e173e OGL_SwapBuffers
void OGL_SwapBuffers(void)
{
    SwapBuffers(g_hDC);
}

// A QuickDraw Point: v (y) first, then h (x).
struct OGLMacPoint {
    int16_t v;
    int16_t h;
};
static_assert(sizeof(OGLMacPoint) == 4, "OGLMacPoint");

// The part of a Carbon EventRecord the mouse clamp reads.
struct OGLMacEventRecord {
    uint16_t what;          // +0x0
    uint32_t message;       // +0x2
    uint32_t when;          // +0x6
    OGLMacPoint where;      // +0xa
} __attribute__((packed));
static_assert(offsetof(OGLMacEventRecord, where) == 0xa, "OGLMacEventRecord.where");

// Mac 002e175d OGL_MacClampMouseEvent
int OGL_MacClampMouseEvent(void *pEvent)
{
    OGLMacEventRecord *pRecord = static_cast<OGLMacEventRecord *>(pEvent);
    if (g_bSurfacePaused == 0 && g_bFullscreen != 0 && pRecord->what < 9) {
        OGLMacPoint pt = pRecord->where;
        int16_t nOriginH = (int16_t)g_ptWindowOrigin.x;
        int16_t nOriginV = (int16_t)g_ptWindowOrigin.y;
        OGLMacPoint ptClamped = pt;
        if (ptClamped.h < nOriginH)
            ptClamped.h = nOriginH;
        if (ptClamped.v < nOriginV)
            ptClamped.v = nOriginV;
        int nMaxH = (int16_t)((int16_t)g_nScreenWidth + nOriginH) - 1;
        if (nMaxH < ptClamped.h)
            ptClamped.h = (int16_t)nMaxH;
        int nMaxV = (int16_t)((int16_t)g_nScreenHeight + nOriginV) - 1;
        if (nMaxV < ptClamped.v)
            ptClamped.v = (int16_t)nMaxV;
        if (ptClamped.v != pt.v || ptClamped.h != pt.h) {
            SetCursorPos(ptClamped.h, ptClamped.v);
            pRecord->where = ptClamped;
        }
    }
    return 0;
}

// Mac 002e185d OGL_MacGlobalToLocal
BOOL OGL_MacGlobalToLocal(void *pPoint)
{
    if (g_bSurfacePaused != 0)
        return FALSE;
    OGLMacPoint *pPt = static_cast<OGLMacPoint *>(pPoint);
    pPt->h = (int16_t)(pPt->h - (int16_t)g_ptWindowOrigin.x);
    pPt->v = (int16_t)(pPt->v - (int16_t)g_ptWindowOrigin.y);
    return TRUE;
}

// Mac 002e1894 OGL_MacLocalToGlobal
BOOL OGL_MacLocalToGlobal(void *pPoint)
{
    if (g_bSurfacePaused != 0)
        return FALSE;
    OGLMacPoint *pPt = static_cast<OGLMacPoint *>(pPoint);
    pPt->h = (int16_t)(pPt->h + (int16_t)g_ptWindowOrigin.x);
    pPt->v = (int16_t)(pPt->v + (int16_t)g_ptWindowOrigin.y);
    return TRUE;
}

// Mac 002e18cb OGL_ActivateWindow
BOOL OGL_ActivateWindow(BOOL bActive)
{
    return bActive != 0;
}

// Mac 002e18da OGL_SetGammaLevel
void OGL_SetGammaLevel(int nGamma)
{
    int nClamped = 0x37;
    if (nGamma > 0x36)
        nClamped = nGamma;
    g_nGamma = 0xff;
    if (nClamped < 0x100)
        g_nGamma = nClamped;
    OGL_ApplyGamma();
}

// Mac 002e190b OGL_ApplyGamma
void OGL_ApplyGamma(void)
{
    if (g_bFullscreen == 1)
        Platform_SetDisplayGamma(g_hDisplay, g_nGamma, g_nContrast);
}

// Mac 002e194a OGL_GammaBlack
void OGL_GammaBlack(void)
{
    if (g_bFullscreen == 1)
        Platform_SetDisplayGamma(g_hDisplay, 0, 100);
}

// Mac 002e1985 OGL_GammaIdentity
void OGL_GammaIdentity(void)
{
    if (g_bFullscreen == 1)
        Platform_SetDisplayGamma(g_hDisplay, 100, 100);
}

// Mac 002e19c0 OGL_RestoreGamma
void OGL_RestoreGamma(void)
{
    if (g_bFullscreen == 1)
        Platform_RestoreDisplayGamma(g_hDisplay);
}

// Mac 002e19eb OGL_ClearDisplay
void OGL_ClearDisplay(void)
{
    // The Mac fills the whole captured display black through CoreGraphics; the context covers the
    // display here, so a full clear and a swap do the same.
    if (g_bFullscreen == 1) {
        glClear(GL_COLOR_BUFFER_BIT);
        SwapBuffers(g_hDC);
    }
}

// Mac 002e1b4c OGL_StartCutscene
BOOL OGL_StartCutscene(BYTE **ppBuffer, int *pnPitch, int *pUnused)
{
    (void)pUnused;
    g_bCutsceneActive = true;
    int nHeight = 480;
    if (g_nResolution == 1)
        nHeight = 600;
    int nWidth = 640;
    if (g_nResolution == 1)
        nWidth = 800;
    if (g_pCutsceneBuffer == NULL)
        g_pCutsceneBuffer = static_cast<BYTE *>(OGL_StormNewArray(0x12c000));
    OGL_CutsceneBeginGL(nWidth, nHeight, 640, 480);
    *ppBuffer = g_pCutsceneBuffer;
    *pnPitch = 0xa00;
    g_nBinkSurfaceType = Platform_GetBinkSurfaceType(g_hDisplay);
    return TRUE;
}

// Mac 002e1bf5 OGL_StopCutscene
BOOL OGL_StopCutscene(void)
{
    if (g_bCutsceneActive == true) {
        OGL_CutsceneRestoreGL();
        if (g_pCutsceneBuffer != NULL) {
            OGL_StormDeleteArray(g_pCutsceneBuffer);
            g_pCutsceneBuffer = NULL;
        }
        g_bCutsceneActive = false;
    }
    return TRUE;
}

// Mac 002e1c3c OGL_SetOption
BOOL OGL_SetOption(int nOption, int nValue)
{
    void *pValue = reinterpret_cast<void *>(static_cast<intptr_t>(nValue));
    switch (nOption) {
    case 0:
        g_hDisplay = static_cast<HMONITOR>(pValue);
        break;
    case 1:
        *static_cast<int *>(pValue) = (int)reinterpret_cast<intptr_t>(g_hDisplay);
        break;
    case 2:
        g_aDisplayModes[0] = *static_cast<const OGLDisplayModeRec *>(pValue);
        break;
    case 3:
        g_aDisplayModes[1] = *static_cast<const OGLDisplayModeRec *>(pValue);
        g_aDisplayModes[2] = *static_cast<const OGLDisplayModeRec *>(pValue);
        break;
    case 4:
        g_nRendererId = nValue;
        break;
    case 5:
        g_nColorBits = 16;
        if (nValue != 0)
            g_nColorBits = 32;
        break;
    case 8:
        g_bFullscreen = (nValue == 0);
        break;
    case 10:
        *static_cast<int *>(pValue) = g_bInScene;
        break;
    case 0xb:
        g_nContrast = nValue;
        return FALSE;
    default:
        return FALSE;
    }
    return TRUE;
}

// Mac 002e70fc OGL_FindDisplayDevice
HMONITOR OGL_FindDisplayDevice(HMONITOR hDisplay)
{
    if (hDisplay != NULL)
        return hDisplay;
    POINT ptOrigin = {0, 0};
    return MonitorFromPoint(ptOrigin, MONITOR_DEFAULTTOPRIMARY);
}

// ---- platform services

// Switch to another ready thread (pthread_yield_np).
void Platform_Yield(void)
{
    SwitchToThread();
}

// The input messages of one Carbon event mask class.
struct PlatformEventClass {
    uint16_t wMask;
    UINT aMessages[8];
};

static const PlatformEventClass s_aEventClasses[] = {
    // mouseDown
    {0x02, {WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, WM_RBUTTONDOWN, WM_RBUTTONDBLCLK, WM_MBUTTONDOWN,
            WM_MBUTTONDBLCLK, WM_XBUTTONDOWN, WM_XBUTTONDBLCLK}},
    // mouseUp
    {0x04, {WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP, WM_XBUTTONUP, 0, 0, 0, 0}},
    // keyDown, autoKey (Windows repeats are WM_KEYDOWN too)
    {0x08 | 0x20, {WM_KEYDOWN, WM_SYSKEYDOWN, WM_CHAR, WM_SYSCHAR, 0, 0, 0, 0}},
    // keyUp
    {0x10, {WM_KEYUP, WM_SYSKEYUP, 0, 0, 0, 0, 0, 0}},
};

// Discard the pending input messages of the Carbon mask classes; never other messages.
void Platform_FlushEvents(uint16_t wCarbonMask)
{
    MSG msg;
    for (const PlatformEventClass &eventClass : s_aEventClasses) {
        if (!(eventClass.wMask & wCarbonMask))
            continue;
        for (UINT nMessage : eventClass.aMessages) {
            if (nMessage == 0)
                break;
            while (PeekMessageA(&msg, NULL, nMessage, nMessage, PM_REMOVE)) {
            }
        }
    }
}

// Is an input message of the Carbon mask classes pending (left in the queue).
BOOL Platform_EventAvail(uint16_t wCarbonMask)
{
    MSG msg;
    BOOL bAvail = FALSE;
    for (const PlatformEventClass &eventClass : s_aEventClasses) {
        if (!(eventClass.wMask & wCarbonMask))
            continue;
        for (UINT nMessage : eventClass.aMessages) {
            if (nMessage == 0)
                break;
            if (PeekMessageA(&msg, NULL, nMessage, nMessage, PM_NOREMOVE))
                bAvail = TRUE;
        }
    }
    return bAvail;
}

// The cursor position in screen coordinates.
void Platform_GetCursorPos(POINT *pPt)
{
    if (!GetCursorPos(pPt)) {
        pPt->x = 0;
        pPt->y = 0;
    }
}

// The window's client origin in screen coordinates.
void Platform_GetClientOrigin(POINT *pPt)
{
    pPt->x = 0;
    pPt->y = 0;
    ClientToScreen(g_hWnd, pPt);
}

// The display's own ramp, saved by the first ramp change (Mac 00553894 flag, 00553898 tables).
static bool s_bGammaSaved;
static WORD s_awSavedGammaRamp[3][256];
static bool s_bGammaFailureLogged;

// Build the Fog gamma ramp and load it into the display; save the display's own ramp first.
void Platform_SetDisplayGamma(HMONITOR hDisplay, int nGamma, int nContrast)
{
    HDC hDC = Platform_OpenDisplayDC(hDisplay);
    if (hDC == NULL)
        return;
    if (!s_bGammaSaved && GetDeviceGammaRamp(hDC, s_awSavedGammaRamp))
        s_bGammaSaved = true;

    // The D3D renderer's call (006b3570): 2-byte entries, 256 of them, up to 0xffff.
    WORD awRamp[3][256];
    g_host.BuildGammaRamp(nGamma, reinterpret_cast<uint32_t *>(awRamp[0]), (double)nContrast,
                          reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(2)), 0x100, 0xffff);
    memcpy(awRamp[1], awRamp[0], sizeof(awRamp[0]));
    memcpy(awRamp[2], awRamp[0], sizeof(awRamp[0]));
    if (!SetDeviceGammaRamp(hDC, awRamp) && !s_bGammaFailureLogged) {
        s_bGammaFailureLogged = true;
        d2log("platform: SetDeviceGammaRamp(gamma %d, contrast %d) rejected", nGamma, nContrast);
    }
    Platform_CloseDisplayDC(hDisplay, hDC);
}

// Load the saved ramp back into the display.
void Platform_RestoreDisplayGamma(HMONITOR hDisplay)
{
    if (!s_bGammaSaved)
        return;
    HDC hDC = Platform_OpenDisplayDC(hDisplay);
    if (hDC == NULL)
        return;
    SetDeviceGammaRamp(hDC, s_awSavedGammaRamp);
    Platform_CloseDisplayDC(hDisplay, hDC);
    s_bGammaSaved = false;
}

// Mac 00036ffe: the mode with the smallest (area over the request + 100 for a stretch mismatch +
// refresh difference) among the modes at least nWidth x nHeight with nColorBits.
BOOL Platform_FindDisplayMode(HMONITOR hDisplay, int nWidth, int nHeight, int nColorBits, int nRefreshHz,
                              BOOL bStretched, BOOL bAcceptClosest, DEVMODEA *pMode)
{
    MONITORINFOEXA info;
    const char *szDevice = Platform_DisplayDeviceName(hDisplay, &info);
    int nBestScore = 0x7fffffff;
    bool bFound = false;
    DEVMODEA dm;
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsA(szDevice, i, &dm); i++) {
        if (dm.dmBitsPerPel != (DWORD)nColorBits)
            continue;
        uint32_t nModeWidth = dm.dmPelsWidth;
        uint32_t nModeHeight = dm.dmPelsHeight;
        if ((uint32_t)nWidth > nModeWidth || (uint32_t)nHeight > nModeHeight)
            continue;
        int nRefreshDiff = nRefreshHz - (int)dm.dmDisplayFrequency;
        if (nRefreshDiff < 0)
            nRefreshDiff = -nRefreshDiff;
        // Windows modes carry no stretched flag.
        int nStretchPenalty = (bStretched != 0) ? 100 : 0;
        int nScore = (int)(nModeHeight * nModeWidth - (uint32_t)nHeight * (uint32_t)nWidth) + nStretchPenalty +
                     nRefreshDiff;
        if (nScore < nBestScore) {
            nBestScore = nScore;
            *pMode = dm;
            bFound = true;
        }
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
    }
    return bFound && (bAcceptClosest || nBestScore == 0);
}

// The Mac's 250 ms settle: at least ten 10 ms sleeps and at least 250 ms.
void Platform_SettleWait(void)
{
    DWORD dwStart = GetTickCount();
    for (unsigned i = 0;; i++) {
        if (i > 9 && (int)(GetTickCount() - 0xfa - dwStart) >= 0)
            return;
        Sleep(10);
    }
}

// The Bink surface type for the cutscene buffer. The GL context is always 32-bit, so this is Bink's 32-bit
// type whatever the desktop reports (Windows tells Diablo II's Game.exe it runs at 16 bits). The game's
// binkw32.dll is Bink 1.0f, where 32-bit is type 2 and writes B, G, R, 0.
int Platform_GetBinkSurfaceType(HMONITOR hDisplay)
{
    (void)hDisplay;
    return 2;
}

// The GL pixel format of the buffer Platform_GetBinkSurfaceType's type produces.
GLenum Platform_GetBinkPixelFormat(void)
{
    return GL_BGRA_EXT;
}

// Wait until GL is done with the texture (no per-object finish in GL 1.1).
void Platform_FinishTextureObject(GLuint nTexture)
{
    (void)nTexture;
    glFinish();
}

typedef void(APIENTRY *PFNPLATFORMGLACTIVETEXTURE)(GLenum eTexture);
static PFNPLATFORMGLACTIVETEXTURE s_pfnGlActiveTexture;

// glActiveTexture through wglGetProcAddress (needs a current context); a no-op when absent.
void Platform_glActiveTexture(GLenum eTexture)
{
    if (s_pfnGlActiveTexture == NULL) {
        PROC pfn = wglGetProcAddress("glActiveTexture");
        if (pfn == NULL)
            pfn = wglGetProcAddress("glActiveTextureARB");
        uintptr_t nAddress = reinterpret_cast<uintptr_t>(pfn);
        if (nAddress == 0 || nAddress == 1 || nAddress == 2 || nAddress == 3 || nAddress == (uintptr_t)-1)
            return;
        s_pfnGlActiveTexture =
            reinterpret_cast<PFNPLATFORMGLACTIVETEXTURE>(reinterpret_cast<void (*)(void)>(pfn));
    }
    s_pfnGlActiveTexture(eTexture);
}

static void (*s_apfnAtExit[8])(void);
static int s_nAtExit;

// Store an atexit function for platform_run_atexit.
int platform_atexit(void (*pfnFunc)(void))
{
    if (s_nAtExit >= (int)(sizeof(s_apfnAtExit) / sizeof(s_apfnAtExit[0])))
        return -1;
    s_apfnAtExit[s_nAtExit++] = pfnFunc;
    return 0;
}

// Run the stored atexit functions, last in first out (DllMain, FreeLibrary only).
extern "C" void platform_run_atexit(void)
{
    while (s_nAtExit > 0) {
        void (*pfnFunc)(void) = s_apfnAtExit[--s_nAtExit];
        s_apfnAtExit[s_nAtExit] = NULL;
        if (pfnFunc)
            pfnFunc();
    }
}
