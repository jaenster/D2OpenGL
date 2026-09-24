// The GL half of the present stage: the game-sized framebuffer object the renderer draws into, and the
// swap that draws it into the window. The renderer's own GL state is left as it was: the swap saves and
// restores everything it touches, and the renderer only ever sees its framebuffer object bound (so its
// glReadPixels screen grab reads the game-sized image).

#include <GL/gl.h>

#include "internal.h"
#include "present.h"

#include "../../common/log.h"

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
int s_nWidth;
int s_nHeight;
GLuint s_nFramebuffer;
GLuint s_nTexture;
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

bool CreateObjects(int nWidth, int nHeight)
{
    glPushAttrib(GL_TEXTURE_BIT);
    glGenTextures(1, &s_nTexture);
    glBindTexture(GL_TEXTURE_2D, s_nTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kClampToEdge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kClampToEdge);
    glTexImage2D(GL_TEXTURE_2D, 0, kRgba8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glPopAttrib();

    s_gl.GenFramebuffers(1, &s_nFramebuffer);
    s_gl.BindFramebuffer(kFramebuffer, s_nFramebuffer);
    s_gl.FramebufferTexture2D(kFramebuffer, kColorAttachment0, GL_TEXTURE_2D, s_nTexture, 0);
    GLenum eStatus = s_gl.CheckFramebufferStatus(kFramebuffer);
    if (eStatus != kFramebufferComplete) {
        d2log("present: framebuffer incomplete (%04x)", eStatus);
        return false;
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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
    s_gl.Uniform2f(s_gl.GetUniformLocation(s_nProgram, "uGameSize"), (float)s_nWidth, (float)s_nHeight);
    s_gl.Uniform2f(s_gl.GetUniformLocation(s_nProgram, "uImageSize"), (float)image.nWidth, (float)image.nHeight);
    if (cfg.eShader == SHADER_CRT) {
        s_gl.Uniform1f(s_gl.GetUniformLocation(s_nProgram, "uScanlines"), cfg.fScanlines);
        s_gl.Uniform1f(s_gl.GetUniformLocation(s_nProgram, "uMask"), cfg.fMask);
        s_gl.Uniform1f(s_gl.GetUniformLocation(s_nProgram, "uGlow"), cfg.fGlow);
    }
}

// Draws the game image into the bound default framebuffer.
void DrawImage(void)
{
    PresentImage image;
    int nClientWidth, nClientHeight;
    PresentWindow_GetImage(&image, &nClientWidth, &nClientHeight);

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

    // The bars.
    glViewport(0, 0, nClientWidth, nClientHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // GL's window origin is the bottom-left corner.
    glViewport(image.nX, nClientHeight - image.nY - image.nHeight, image.nWidth, image.nHeight);
    s_gl.ActiveTexture(kTexture0);
    if (s_bTextureRectangle)
        glDisable(kTextureRectangle);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_nTexture);
    GLint nFilter = g_presentConfig.eFilter == FILTER_NEAREST && !s_nProgram ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nFilter);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (s_nProgram) {
        s_gl.UseProgram(s_nProgram);
        SetUniforms(image);
    }

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glEnd();

    if (s_nProgram)
        s_gl.UseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

}  // namespace

bool Present_KeepsDisplayMode(void)
{
    return g_presentConfig.eScaling != SCALING_OFF && g_presentConfig.bBorderless && !s_bFailed;
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

bool Present_SwapBuffers(HDC hDC)
{
    if (!s_bOpen)
        return false;
    s_gl.BindFramebuffer(kFramebuffer, 0);
    DrawImage();
    SwapBuffers(hDC);
    s_gl.BindFramebuffer(kFramebuffer, s_nFramebuffer);
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
