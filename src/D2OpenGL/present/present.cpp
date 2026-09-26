// The GL half of the present stage: the framebuffer object the renderer draws into (the game size times
// the render scale), and the swap that draws it into the window. The renderer's own GL state is left as
// it was: the swap saves and restores everything it touches, and the renderer only ever sees its
// framebuffer object bound. Its glReadPixels screen grab reads a game-sized copy (Present_BeginReadBack).

#include <GL/gl.h>

#include "internal.h"
#include "present.h"

#include "../../common/log.h"
#include "../renderer/renderer.h"
#include "../upscale/sprite_upscale.h"

namespace {

// GL 2.0 / 3.0 names and entry points (opengl32.dll exports GL 1.1 only).
typedef char GLcharPresent;
const GLenum kFramebuffer = 0x8D40;
const GLenum kColorAttachment0 = 0x8CE0;
const GLenum kFramebufferComplete = 0x8CD5;
const GLenum kFragmentShader = 0x8B30;
const GLenum kVertexShader = 0x8B31;
const GLenum kCompileStatus = 0x8B81;
const GLenum kLinkStatus = 0x8B82;
const GLenum kClampToEdge = 0x812F;
const GLenum kTexture0 = 0x84C0;
const GLenum kRgba8 = 0x8058;
const GLenum kTextureRectangle = 0x84F5;

struct PresentGL {
    void(APIENTRY *GenFramebuffers)(GLsizei, GLuint *);
    void(APIENTRY *DeleteFramebuffers)(GLsizei, const GLuint *);
    void(APIENTRY *BindFramebuffer)(GLenum, GLuint);
    void(APIENTRY *FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
    GLenum(APIENTRY *CheckFramebufferStatus)(GLenum);
    void(APIENTRY *ActiveTexture)(GLenum);
    GLuint(APIENTRY *CreateShader)(GLenum);
    void(APIENTRY *DeleteShader)(GLuint);
    void(APIENTRY *ShaderSource)(GLuint, GLsizei, const GLcharPresent *const *, const GLint *);
    void(APIENTRY *CompileShader)(GLuint);
    void(APIENTRY *GetShaderiv)(GLuint, GLenum, GLint *);
    void(APIENTRY *GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLcharPresent *);
    GLuint(APIENTRY *CreateProgram)(void);
    void(APIENTRY *DeleteProgram)(GLuint);
    void(APIENTRY *AttachShader)(GLuint, GLuint);
    void(APIENTRY *LinkProgram)(GLuint);
    void(APIENTRY *GetProgramiv)(GLuint, GLenum, GLint *);
    void(APIENTRY *GetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLcharPresent *);
    void(APIENTRY *UseProgram)(GLuint);
    GLint(APIENTRY *GetUniformLocation)(GLuint, const GLcharPresent *);
    void(APIENTRY *Uniform1i)(GLint, GLint);
    void(APIENTRY *Uniform1f)(GLint, GLfloat);
    void(APIENTRY *Uniform2f)(GLint, GLfloat, GLfloat);
};

PresentGL s_gl;
bool s_bFailed;  // the stage could not run on this machine: the renderer draws straight to the window
bool s_bOpen;
int s_nWidth;   // the game size
int s_nHeight;
int s_nRenderScale = 1;
GLuint s_nFramebuffer;
GLuint s_nTexture;
GLuint s_nReadFramebuffer;  // render scale above 1: the game-sized copy the screen read reads
GLuint s_nReadTexture;
GLuint s_nProgram;
bool s_bTextureRectangle;
unsigned s_nFrame;

// wglGetProcAddress, with the core name first; 0, 1, 2, 3 and -1 mean absent.
template <typename Fn> bool Load(Fn *ppfn, const char *szName, const char *szAlternative)
{
    PROC pfn = wglGetProcAddress(szName);
    uintptr_t n = reinterpret_cast<uintptr_t>(pfn);
    if ((n <= 3 || n == (uintptr_t)-1) && szAlternative) {
        pfn = wglGetProcAddress(szAlternative);
        n = reinterpret_cast<uintptr_t>(pfn);
    }
    if (n <= 3 || n == (uintptr_t)-1) {
        *ppfn = NULL;
        return false;
    }
    *ppfn = reinterpret_cast<Fn>(reinterpret_cast<void (*)(void)>(pfn));
    return true;
}

bool LoadFramebufferEntryPoints(void)
{
    bool bOk = true;
    bOk &= Load(&s_gl.GenFramebuffers, "glGenFramebuffers", "glGenFramebuffersEXT");
    bOk &= Load(&s_gl.DeleteFramebuffers, "glDeleteFramebuffers", "glDeleteFramebuffersEXT");
    bOk &= Load(&s_gl.BindFramebuffer, "glBindFramebuffer", "glBindFramebufferEXT");
    bOk &= Load(&s_gl.FramebufferTexture2D, "glFramebufferTexture2D", "glFramebufferTexture2DEXT");
    bOk &= Load(&s_gl.CheckFramebufferStatus, "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    bOk &= Load(&s_gl.ActiveTexture, "glActiveTexture", "glActiveTextureARB");
    return bOk;
}

bool LoadShaderEntryPoints(void)
{
    bool bOk = true;
    bOk &= Load(&s_gl.CreateShader, "glCreateShader", NULL);
    bOk &= Load(&s_gl.DeleteShader, "glDeleteShader", NULL);
    bOk &= Load(&s_gl.ShaderSource, "glShaderSource", NULL);
    bOk &= Load(&s_gl.CompileShader, "glCompileShader", NULL);
    bOk &= Load(&s_gl.GetShaderiv, "glGetShaderiv", NULL);
    bOk &= Load(&s_gl.GetShaderInfoLog, "glGetShaderInfoLog", NULL);
    bOk &= Load(&s_gl.CreateProgram, "glCreateProgram", NULL);
    bOk &= Load(&s_gl.DeleteProgram, "glDeleteProgram", NULL);
    bOk &= Load(&s_gl.AttachShader, "glAttachShader", NULL);
    bOk &= Load(&s_gl.LinkProgram, "glLinkProgram", NULL);
    bOk &= Load(&s_gl.GetProgramiv, "glGetProgramiv", NULL);
    bOk &= Load(&s_gl.GetProgramInfoLog, "glGetProgramInfoLog", NULL);
    bOk &= Load(&s_gl.UseProgram, "glUseProgram", NULL);
    bOk &= Load(&s_gl.GetUniformLocation, "glGetUniformLocation", NULL);
    bOk &= Load(&s_gl.Uniform1i, "glUniform1i", NULL);
    bOk &= Load(&s_gl.Uniform1f, "glUniform1f", NULL);
    bOk &= Load(&s_gl.Uniform2f, "glUniform2f", NULL);
    return bOk;
}

GLuint CompileShader(GLenum eType, const char *szSource)
{
    GLuint nShader = s_gl.CreateShader(eType);
    s_gl.ShaderSource(nShader, 1, &szSource, NULL);
    s_gl.CompileShader(nShader);
    GLint nStatus = 0;
    s_gl.GetShaderiv(nShader, kCompileStatus, &nStatus);
    if (!nStatus) {
        char szLog[1024] = "";
        s_gl.GetShaderInfoLog(nShader, sizeof(szLog), NULL, szLog);
        d2log("present: shader does not compile: %s", szLog);
        s_gl.DeleteShader(nShader);
        return 0;
    }
    return nShader;
}

GLuint BuildProgram(const char *szFragment)
{
    GLuint nVertex = CompileShader(kVertexShader, g_szPresentVertexShader);
    GLuint nFragment = nVertex ? CompileShader(kFragmentShader, szFragment) : 0;
    if (!nFragment) {
        if (nVertex)
            s_gl.DeleteShader(nVertex);
        return 0;
    }
    GLuint nProgram = s_gl.CreateProgram();
    s_gl.AttachShader(nProgram, nVertex);
    s_gl.AttachShader(nProgram, nFragment);
    s_gl.LinkProgram(nProgram);
    s_gl.DeleteShader(nVertex);
    s_gl.DeleteShader(nFragment);
    GLint nStatus = 0;
    s_gl.GetProgramiv(nProgram, kLinkStatus, &nStatus);
    if (!nStatus) {
        char szLog[1024] = "";
        s_gl.GetProgramInfoLog(nProgram, sizeof(szLog), NULL, szLog);
        d2log("present: shader does not link: %s", szLog);
        s_gl.DeleteProgram(nProgram);
        return 0;
    }
    return nProgram;
}

bool HasExtension(const char *szName)
{
    const char *szExtensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    return szExtensions && strstr(szExtensions, szName);
}

// GL errors this stage raised; never left for the renderer's own glGetError checks.
void ClearErrors(const char *szWhere)
{
    static bool s_bLogged;
    GLenum eError;
    while ((eError = glGetError()) != GL_NO_ERROR) {
        if (!s_bLogged) {
            s_bLogged = true;
            d2log("present: GL error %04x in %s", eError, szWhere);
        }
    }
}

void DeleteObjects(void)
{
    if (s_nReadFramebuffer) {
        s_gl.BindFramebuffer(kFramebuffer, 0);
        s_gl.DeleteFramebuffers(1, &s_nReadFramebuffer);
        s_nReadFramebuffer = 0;
    }
    if (s_nReadTexture) {
        glDeleteTextures(1, &s_nReadTexture);
        s_nReadTexture = 0;
    }
    if (s_nProgram) {
        s_gl.UseProgram(0);
        s_gl.DeleteProgram(s_nProgram);
        s_nProgram = 0;
    }
    if (s_nFramebuffer) {
        s_gl.BindFramebuffer(kFramebuffer, 0);
        s_gl.DeleteFramebuffers(1, &s_nFramebuffer);
        s_nFramebuffer = 0;
    }
    if (s_nTexture) {
        glDeleteTextures(1, &s_nTexture);
        s_nTexture = 0;
    }
}

// A colour texture of this size and a framebuffer object drawing into it, cleared to black.
bool CreateTarget(GLuint *pnTexture, GLuint *pnFramebuffer, int nWidth, int nHeight)
{
    glPushAttrib(GL_TEXTURE_BIT);
    glGenTextures(1, pnTexture);
    glBindTexture(GL_TEXTURE_2D, *pnTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kClampToEdge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kClampToEdge);
    glTexImage2D(GL_TEXTURE_2D, 0, kRgba8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glPopAttrib();

    s_gl.GenFramebuffers(1, pnFramebuffer);
    s_gl.BindFramebuffer(kFramebuffer, *pnFramebuffer);
    s_gl.FramebufferTexture2D(kFramebuffer, kColorAttachment0, GL_TEXTURE_2D, *pnTexture, 0);
    GLenum eStatus = s_gl.CheckFramebufferStatus(kFramebuffer);
    if (eStatus != kFramebufferComplete) {
        d2log("present: framebuffer incomplete (%04x) at %dx%d", eStatus, nWidth, nHeight);
        return false;
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return true;
}

// The render scale this GL can draw at: the buffer must fit the texture and viewport limits.
int FitRenderScale(int nWidth, int nHeight, int nScale)
{
    GLint nMaxTexture = 0;
    GLint anMaxViewport[2] = {0, 0};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &nMaxTexture);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, anMaxViewport);
    int nFit = nScale;
    while (nFit > 1 && (nWidth * nFit > nMaxTexture || nHeight * nFit > nMaxTexture ||
                        nWidth * nFit > anMaxViewport[0] || nHeight * nFit > anMaxViewport[1]))
        --nFit;
    if (nFit != nScale)
        d2log("present: RenderScale=%d does not fit (texture %d, viewport %dx%d); %d used", nScale, nMaxTexture,
              anMaxViewport[0], anMaxViewport[1], nFit);
    return nFit;
}

bool CreateObjects(int nWidth, int nHeight)
{
    if (s_nRenderScale > 1 && !CreateTarget(&s_nReadTexture, &s_nReadFramebuffer, nWidth, nHeight))
        return false;
    if (!CreateTarget(&s_nTexture, &s_nFramebuffer, nWidth * s_nRenderScale, nHeight * s_nRenderScale))
        return false;
    // The renderer's one-pixel lines and points keep their weight in game pixels.
    glLineWidth((GLfloat)s_nRenderScale);
    glPointSize((GLfloat)s_nRenderScale);

    const PresentConfig &cfg = g_presentConfig;
    bool bNeedsShader = cfg.eShader == SHADER_CRT || cfg.eFilter == FILTER_SHARP;
    if (bNeedsShader) {
        if (!LoadShaderEntryPoints()) {
            d2log("present: no GLSL; linear filter, no shader");
        } else {
            s_nProgram = BuildProgram(cfg.eShader == SHADER_CRT ? g_szPresentCrtShader : g_szPresentSharpShader);
            if (!s_nProgram)
                d2log("present: shader unavailable; linear filter, no shader");
        }
    }
    return true;
}

void SetUniforms(const PresentImage &image)
{
    const PresentConfig &cfg = g_presentConfig;
    s_gl.Uniform1i(s_gl.GetUniformLocation(s_nProgram, "uTexture"), 0);
    // The sharp filter scales the rendered image, the CRT emulates a screen at the game's resolution.
    int nSourceScale = cfg.eShader == SHADER_CRT ? 1 : s_nRenderScale;
    s_gl.Uniform2f(s_gl.GetUniformLocation(s_nProgram, "uGameSize"), (float)(s_nWidth * nSourceScale),
                   (float)(s_nHeight * nSourceScale));
    s_gl.Uniform2f(s_gl.GetUniformLocation(s_nProgram, "uImageSize"), (float)image.nWidth, (float)image.nHeight);
    if (cfg.eShader == SHADER_CRT) {
        s_gl.Uniform1f(s_gl.GetUniformLocation(s_nProgram, "uScanlines"), cfg.fScanlines);
        s_gl.Uniform1f(s_gl.GetUniformLocation(s_nProgram, "uMask"), cfg.fMask);
        s_gl.Uniform1f(s_gl.GetUniformLocation(s_nProgram, "uGlow"), cfg.fGlow);
    }
}

// Saves every piece of GL state the stage's own drawing changes and sets a plain, untextured,
// unblended state with identity matrices.
void PushState(void)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DITHER);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PopState(void)
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

// The whole frame as a quad in clip space, texture coordinates shifted by (fShiftS, fShiftT).
void DrawQuad(float fShiftS, float fShiftT)
{
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0f + fShiftS, 0.0f + fShiftT);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f + fShiftS, 0.0f + fShiftT);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(0.0f + fShiftS, 1.0f + fShiftT);
    glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(1.0f + fShiftS, 1.0f + fShiftT);
    glVertex2f(1.0f, 1.0f);
    glEnd();
}

// Binds the frame's texture on unit 0 for a textured draw with this filter.
void BindFrameTexture(GLint nFilter)
{
    s_gl.ActiveTexture(kTexture0);
    if (s_bTextureRectangle)
        glDisable(kTextureRectangle);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_nTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nFilter);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Draws the game image into the bound default framebuffer.
void DrawImage(void)
{
    PresentImage image;
    int nClientWidth, nClientHeight;
    PresentWindow_GetImage(&image, &nClientWidth, &nClientHeight);

    PushState();

    // The bars.
    glViewport(0, 0, nClientWidth, nClientHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // GL's window origin is the bottom-left corner.
    glViewport(image.nX, nClientHeight - image.nY - image.nHeight, image.nWidth, image.nHeight);
    BindFrameTexture(g_presentConfig.eFilter == FILTER_NEAREST && !s_nProgram ? GL_NEAREST : GL_LINEAR);
    if (s_nProgram) {
        s_gl.UseProgram(s_nProgram);
        SetUniforms(image);
    }
    DrawQuad(0.0f, 0.0f);
    if (s_nProgram)
        s_gl.UseProgram(0);

    PopState();
}

}  // namespace

bool Present_KeepsDisplayMode(void)
{
    return g_presentConfig.eScaling != SCALING_OFF && g_presentConfig.bBorderless && !s_bFailed;
}

int Present_RenderScale(void)
{
    return s_bOpen ? s_nRenderScale : 1;
}

bool Present_UpscaleSprites(void)
{
    return g_presentConfig.eUpscale == UPSCALE_MMPX;
}

const char *Present_HDPackPath(void)
{
    return g_presentConfig.szHDPack[0] ? g_presentConfig.szHDPack : NULL;
}

void Present_BeginReadBack(void)
{
    if (!s_bOpen || s_nRenderScale == 1)
        return;
    // Each game pixel is read from one pixel of its block (the centre one, or above-left of the centre),
    // never an average: the image keeps the frame's colours.
    int nPick = (s_nRenderScale - 1) / 2;
    float fShiftS = ((float)nPick + 0.5f - 0.5f * (float)s_nRenderScale) / (float)(s_nWidth * s_nRenderScale);
    float fShiftT = ((float)nPick + 0.5f - 0.5f * (float)s_nRenderScale) / (float)(s_nHeight * s_nRenderScale);
    s_gl.BindFramebuffer(kFramebuffer, s_nReadFramebuffer);
    PushState();
    glViewport(0, 0, s_nWidth, s_nHeight);
    BindFrameTexture(GL_NEAREST);
    DrawQuad(fShiftS, fShiftT);
    PopState();
    ClearErrors("read");
}

void Present_EndReadBack(void)
{
    if (!s_bOpen || s_nRenderScale == 1)
        return;
    s_gl.BindFramebuffer(kFramebuffer, s_nFramebuffer);
}

bool Present_Open(HWND hWnd, HDC hDC, int nWidth, int nHeight, bool bFullscreen)
{
    (void)hDC;
    if (g_presentConfig.eScaling == SCALING_OFF || s_bFailed)
        return false;
    if (s_bOpen)
        Present_Close();
    PresentImports_Init();
    present_patch_modules();

    if (!LoadFramebufferEntryPoints()) {
        d2log("present: no framebuffer objects; drawing unscaled");
        s_bFailed = true;
        return false;
    }
    s_bTextureRectangle = HasExtension("GL_ARB_texture_rectangle") || HasExtension("GL_EXT_texture_rectangle") ||
                          HasExtension("GL_NV_texture_rectangle");
    s_nRenderScale = FitRenderScale(nWidth, nHeight, g_presentConfig.nRenderScale);
    if (!CreateObjects(nWidth, nHeight)) {
        DeleteObjects();
        ClearErrors("setup");
        d2log("present: drawing unscaled");
        s_bFailed = true;
        PresentWindow_Detach();
        return false;
    }
    ClearErrors("setup");
    s_nWidth = nWidth;
    s_nHeight = nHeight;
    s_bOpen = true;
    d2log("present: %dx%d, render scale %d, sprites %s", nWidth, nHeight, s_nRenderScale,
          s_nRenderScale > 1 && Present_UpscaleSprites() ? "upscaled (mmpx)" : "not upscaled");
    if (!PresentWindow_Attach(hWnd, nWidth, nHeight, bFullscreen)) {
        Present_Close();
        s_bFailed = true;
        d2log("present: the game window cannot be taken over; drawing unscaled");
        return false;
    }
    return true;
}

void Present_Close(void)
{
    if (!s_bOpen)
        return;
    DeleteObjects();
    ClearErrors("close");
    s_bOpen = false;
}

volatile LONG s_nTogglePending;

void Present_RequestToggle(void)
{
    InterlockedExchange(&s_nTogglePending, 1);
}

bool Present_SwapBuffers(HDC hDC)
{
    if (!s_bOpen)
        return false;
    s_gl.BindFramebuffer(kFramebuffer, 0);
    DrawImage();
    SwapBuffers(hDC);
    s_gl.BindFramebuffer(kFramebuffer, s_nFramebuffer);
    // Between frames, on the render thread: switch the sprite mode and let every texture be made again.
    if (InterlockedExchange(&s_nTogglePending, 0)) {
        Upscale_ToggleOriginal();
        if (g_pTextures)
            g_pTextures->FreeAllTextures();
    }
    ClearErrors("swap");
    // Game modules loaded after the window was made.
    if ((++s_nFrame & 63) == 0)
        present_patch_modules();
    return true;
}

BOOL Present_GetCursorPos(POINT *pPt)
{
    PresentImports_Init();
    BOOL bResult = g_user32.GetCursorPos ? g_user32.GetCursorPos(pPt) : GetCursorPos(pPt);
    if (bResult)
        PresentWindow_ScreenToGame(pPt);
    return bResult;
}
