// The game window under the present stage: its real size, where the image sits in it, and the translation
// of the mouse between real client pixels and game coordinates.
//
// Windowed, the client area is the game size times an integer scale and keeps the game's caption.
// Borderless fullscreen, the window covers its monitor at the desktop resolution and the image is scaled to
// fit it with the aspect kept, black bars around it. The game's window procedure is subclassed: mouse
// messages reach it in game coordinates, WM_SIZE reports the game size, and every size or move the game
// asks for is turned into the real one.

#include "internal.h"
#include "present.h"

#include "../../common/log.h"

namespace {

HWND s_hWnd;
WNDPROC s_pfnGameProc;
bool s_bFullscreen;
bool s_bMapped;
int s_nGameWidth = 800;
int s_nGameHeight = 600;

// Floor division for coordinates that can be negative.
int FloorDiv(int nNum, int nDen)
{
    int q = nNum / nDen;
    return (nNum % nDen != 0 && (nNum < 0) != (nDen < 0)) ? q - 1 : q;
}

void GetMonitorRects(HWND hWnd, RECT *pMonitor, RECT *pWork)
{
    MONITORINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    if (GetMonitorInfoA(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &info)) {
        *pMonitor = info.rcMonitor;
        *pWork = info.rcWork;
    } else {
        SetRect(pMonitor, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        SystemParametersInfoA(SPI_GETWORKAREA, 0, pWork, 0);
    }
}

// The size of the window's frame: the window rectangle less the client area.
void GetFrameSize(HWND hWnd, int *pnWidth, int *pnHeight)
{
    RECT rcWindow, rcClient;
    g_user32.GetWindowRect(hWnd, &rcWindow);
    g_user32.GetClientRect(hWnd, &rcClient);
    *pnWidth = (rcWindow.right - rcWindow.left) - rcClient.right;
    *pnHeight = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
}

// The windowed scale: fixed, or the largest integer whose window fits the work area.
int WindowedScale(HWND hWnd)
{
    if (g_presentConfig.eScaling == SCALING_FIXED)
        return g_presentConfig.nScale;
    RECT rcMonitor, rcWork;
    GetMonitorRects(hWnd, &rcMonitor, &rcWork);
    int nFrameWidth, nFrameHeight;
    GetFrameSize(hWnd, &nFrameWidth, &nFrameHeight);
    int nScaleX = (rcWork.right - rcWork.left - nFrameWidth) / s_nGameWidth;
    int nScaleY = (rcWork.bottom - rcWork.top - nFrameHeight) / s_nGameHeight;
    int nScale = nScaleX < nScaleY ? nScaleX : nScaleY;
    return nScale < 1 ? 1 : nScale;
}

// What the window should be: the monitor (fullscreen) or the scaled client plus the frame (windowed).
void TargetWindowSize(HWND hWnd, int *pnWidth, int *pnHeight)
{
    if (s_bFullscreen) {
        RECT rcMonitor, rcWork;
        GetMonitorRects(hWnd, &rcMonitor, &rcWork);
        *pnWidth = rcMonitor.right - rcMonitor.left;
        *pnHeight = rcMonitor.bottom - rcMonitor.top;
        return;
    }
    int nFrameWidth, nFrameHeight;
    GetFrameSize(hWnd, &nFrameWidth, &nFrameHeight);
    int nScale = WindowedScale(hWnd);
    *pnWidth = s_nGameWidth * nScale + nFrameWidth;
    *pnHeight = s_nGameHeight * nScale + nFrameHeight;
}

// Every size or move the game (or Windows) asks for becomes the real one.
void AdjustWindowPos(HWND hWnd, WINDOWPOS *pPos)
{
    if (IsIconic(hWnd) || (pPos->flags & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE))
        return;
    if (s_bFullscreen) {
        if (!g_presentConfig.bBorderless)
            return;
        RECT rcMonitor, rcWork;
        GetMonitorRects(hWnd, &rcMonitor, &rcWork);
        pPos->x = rcMonitor.left;
        pPos->y = rcMonitor.top;
        pPos->cx = rcMonitor.right - rcMonitor.left;
        pPos->cy = rcMonitor.bottom - rcMonitor.top;
        pPos->flags &= ~(SWP_NOSIZE | SWP_NOMOVE);
        return;
    }
    if (pPos->flags & SWP_NOSIZE)
        return;
    // A client size the game asks for is its new game size (D2gfx resizes the window before the renderer).
    int nFrameWidth, nFrameHeight;
    GetFrameSize(hWnd, &nFrameWidth, &nFrameHeight);
    int nClientWidth = pPos->cx - nFrameWidth;
    int nClientHeight = pPos->cy - nFrameHeight;
    if ((nClientWidth == 640 && nClientHeight == 480) || (nClientWidth == 800 && nClientHeight == 600)) {
        s_nGameWidth = nClientWidth;
        s_nGameHeight = nClientHeight;
    }
    int nWidth, nHeight;
    TargetWindowSize(hWnd, &nWidth, &nHeight);
    if (!(pPos->flags & SWP_NOMOVE)) {
        // Keep the centre the game chose, but the caption on the work area.
        RECT rcMonitor, rcWork;
        GetMonitorRects(hWnd, &rcMonitor, &rcWork);
        pPos->x -= (nWidth - pPos->cx) / 2;
        pPos->y -= (nHeight - pPos->cy) / 2;
        if (pPos->x < rcWork.left)
            pPos->x = rcWork.left;
        if (pPos->y < rcWork.top)
            pPos->y = rcWork.top;
    }
    pPos->cx = nWidth;
    pPos->cy = nHeight;
}

// Borderless: the cursor stays on the image while the game is active, as it does on a switched display.
void ClipToImage(HWND hWnd, bool bActive)
{
    if (!s_bFullscreen || !g_presentConfig.bBorderless)
        return;
    if (!bActive) {
        ClipCursor(NULL);
        return;
    }
    PresentImage image;
    int nClientWidth, nClientHeight;
    PresentWindow_GetImage(&image, &nClientWidth, &nClientHeight);
    POINT ptOrigin = {0, 0};
    ClientToScreen(hWnd, &ptOrigin);
    RECT rcClip = {ptOrigin.x + image.nX, ptOrigin.y + image.nY, ptOrigin.x + image.nX + image.nWidth,
                   ptOrigin.y + image.nY + image.nHeight};
    ClipCursor(&rcClip);
}

// A real client point in game coordinates. Inside the client area it is kept on the image; outside it
// (a drag that left the window) it is scaled like the rest.
POINT ClientToGame(int nX, int nY)
{
    PresentImage image;
    int nClientWidth, nClientHeight;
    PresentWindow_GetImage(&image, &nClientWidth, &nClientHeight);
    POINT pt = {FloorDiv((nX - image.nX) * s_nGameWidth, image.nWidth),
                FloorDiv((nY - image.nY) * s_nGameHeight, image.nHeight)};
    if (nX >= 0 && nY >= 0 && nX < nClientWidth && nY < nClientHeight) {
        if (pt.x < 0)
            pt.x = 0;
        if (pt.y < 0)
            pt.y = 0;
        if (pt.x >= s_nGameWidth)
            pt.x = s_nGameWidth - 1;
        if (pt.y >= s_nGameHeight)
            pt.y = s_nGameHeight - 1;
    }
    return pt;
}

// A game point at the centre of its real pixels.
POINT GameToClient(int nX, int nY)
{
    PresentImage image;
    int nClientWidth, nClientHeight;
    PresentWindow_GetImage(&image, &nClientWidth, &nClientHeight);
    POINT pt = {image.nX + FloorDiv(nX * image.nWidth + image.nWidth / 2, s_nGameWidth),
                image.nY + FloorDiv(nY * image.nHeight + image.nHeight / 2, s_nGameHeight)};
    return pt;
}

LPARAM MakePoint(POINT pt)
{
    return MAKELPARAM((WORD)(short)pt.x, (WORD)(short)pt.y);
}

LRESULT CALLBACK PresentWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pfnGameProc = s_pfnGameProc;
    // Not taken from the game: the key still reaches it (and chat).
    if (nMsg == WM_KEYDOWN && g_presentConfig.nToggleKey && (int)wParam == g_presentConfig.nToggleKey &&
        !(lParam & (1 << 30)))
        Present_RequestToggle();
    if (!s_bMapped)
        return CallWindowProcA(pfnGameProc, hWnd, nMsg, wParam, lParam);
    switch (nMsg) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
        lParam = MakePoint(ClientToGame((short)LOWORD(lParam), (short)HIWORD(lParam)));
        break;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
        POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
        PresentWindow_ScreenToGame(&pt);
        lParam = MakePoint(pt);
        break;
    }
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
            lParam = MAKELPARAM(s_nGameWidth, s_nGameHeight);
        break;
    case WM_WINDOWPOSCHANGING: {
        LRESULT nResult = CallWindowProcA(pfnGameProc, hWnd, nMsg, wParam, lParam);
        AdjustWindowPos(hWnd, reinterpret_cast<WINDOWPOS *>(lParam));
        return nResult;
    }
    case WM_GETMINMAXINFO: {
        LRESULT nResult = CallWindowProcA(pfnGameProc, hWnd, nMsg, wParam, lParam);
        MINMAXINFO *pInfo = reinterpret_cast<MINMAXINFO *>(lParam);
        int nWidth, nHeight;
        TargetWindowSize(hWnd, &nWidth, &nHeight);
        if (pInfo->ptMaxTrackSize.x < nWidth)
            pInfo->ptMaxTrackSize.x = nWidth;
        if (pInfo->ptMaxTrackSize.y < nHeight)
            pInfo->ptMaxTrackSize.y = nHeight;
        return nResult;
    }
    case WM_ACTIVATE:
        ClipToImage(hWnd, LOWORD(wParam) != WA_INACTIVE && !HIWORD(wParam));
        break;
    case WM_DESTROY:
        ClipCursor(NULL);
        s_bMapped = false;
        SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pfnGameProc));
        s_hWnd = NULL;
        break;
    default:
        break;
    }
    return CallWindowProcA(pfnGameProc, hWnd, nMsg, wParam, lParam);
}

}  // namespace

bool PresentWindow_Attach(HWND hWnd, int nGameWidth, int nGameHeight, bool bFullscreen)
{
    if (hWnd == NULL)
        return false;
    if (s_hWnd != hWnd) {
        WNDPROC pfnGameProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrA(hWnd, GWLP_WNDPROC));
        if (pfnGameProc == NULL)
            return false;
        s_pfnGameProc = pfnGameProc;
        s_hWnd = hWnd;
        SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(PresentWindowProc));
    }
    s_nGameWidth = nGameWidth;
    s_nGameHeight = nGameHeight;
    s_bFullscreen = bFullscreen;
    s_bMapped = true;

    if (!bFullscreen || g_presentConfig.bBorderless) {
        int nWidth, nHeight;
        TargetWindowSize(hWnd, &nWidth, &nHeight);
        RECT rcWindow;
        g_user32.GetWindowRect(hWnd, &rcWindow);
        int nX = rcWindow.left - (nWidth - (rcWindow.right - rcWindow.left)) / 2;
        int nY = rcWindow.top - (nHeight - (rcWindow.bottom - rcWindow.top)) / 2;
        SetWindowPos(hWnd, NULL, nX, nY, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    ClipToImage(hWnd, GetForegroundWindow() == hWnd);

    PresentImage image;
    int nClientWidth, nClientHeight;
    PresentWindow_GetImage(&image, &nClientWidth, &nClientHeight);
    d2log("present: game %dx%d, %s client %dx%d, image %dx%d at %d,%d", nGameWidth, nGameHeight,
          bFullscreen ? (g_presentConfig.bBorderless ? "borderless" : "exclusive") : "windowed", nClientWidth,
          nClientHeight, image.nWidth, image.nHeight, image.nX, image.nY);
    return true;
}

void PresentWindow_Detach(void)
{
    if (s_bMapped)
        ClipCursor(NULL);
    s_bMapped = false;
}

// The image rectangle: centred; an integer multiple of the game size, except in borderless fullscreen with
// Scaling=auto, where it fills the height or width of the monitor. Smaller than the game size only when
// the client is.
void PresentWindow_GetImage(PresentImage *pImage, int *pnClientWidth, int *pnClientHeight)
{
    RECT rcClient = {0, 0, s_nGameWidth, s_nGameHeight};
    if (s_hWnd)
        g_user32.GetClientRect(s_hWnd, &rcClient);
    int nClientWidth = rcClient.right > 0 ? rcClient.right : 1;
    int nClientHeight = rcClient.bottom > 0 ? rcClient.bottom : 1;
    *pnClientWidth = nClientWidth;
    *pnClientHeight = nClientHeight;

    int nWidth, nHeight;
    bool bFill = s_bFullscreen && g_presentConfig.bBorderless && g_presentConfig.eScaling == SCALING_AUTO;
    int nScale = nClientWidth / s_nGameWidth < nClientHeight / s_nGameHeight ? nClientWidth / s_nGameWidth
                                                                              : nClientHeight / s_nGameHeight;
    if (g_presentConfig.eScaling == SCALING_FIXED && nScale > g_presentConfig.nScale)
        nScale = g_presentConfig.nScale;
    if (bFill || nScale < 1) {
        // Fit, aspect kept.
        if ((long long)nClientWidth * s_nGameHeight <= (long long)nClientHeight * s_nGameWidth) {
            nWidth = nClientWidth;
            nHeight = (int)((long long)nClientWidth * s_nGameHeight / s_nGameWidth);
        } else {
            nHeight = nClientHeight;
            nWidth = (int)((long long)nClientHeight * s_nGameWidth / s_nGameHeight);
        }
    } else {
        nWidth = s_nGameWidth * nScale;
        nHeight = s_nGameHeight * nScale;
    }
    pImage->nWidth = nWidth;
    pImage->nHeight = nHeight;
    pImage->nX = (nClientWidth - nWidth) / 2;
    pImage->nY = (nClientHeight - nHeight) / 2;
}

bool PresentWindow_IsMapped(HWND hWnd)
{
    return s_bMapped && hWnd == s_hWnd && hWnd != NULL;
}

int PresentWindow_GameWidth(void)
{
    return s_nGameWidth;
}

int PresentWindow_GameHeight(void)
{
    return s_nGameHeight;
}

void PresentWindow_ScreenToGame(POINT *pPt)
{
    if (!s_bMapped)
        return;
    POINT ptOrigin = {0, 0};
    ClientToScreen(s_hWnd, &ptOrigin);
    POINT pt = ClientToGame(pPt->x - ptOrigin.x, pPt->y - ptOrigin.y);
    pPt->x = ptOrigin.x + pt.x;
    pPt->y = ptOrigin.y + pt.y;
}

void PresentWindow_GameToScreen(POINT *pPt)
{
    if (!s_bMapped)
        return;
    POINT ptOrigin = {0, 0};
    ClientToScreen(s_hWnd, &ptOrigin);
    POINT pt = GameToClient(pPt->x - ptOrigin.x, pPt->y - ptOrigin.y);
    pPt->x = ptOrigin.x + pt.x;
    pPt->y = ptOrigin.y + pt.y;
}
