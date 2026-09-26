// D2OpenGL.ini next to the DLL, read once. A missing file, key or unknown value gives the default.
//
//   [Display]
//   Scaling    = auto | off | <integer>     auto
//   Fullscreen = exclusive | borderless     exclusive
//   Filter     = sharp | nearest | linear   sharp
//   Shader     = none | crt                 none
//   RenderScale = 1 | 2 | 3 | 4             2 (1 with Scaling=off)
//   Upscale    = none | mmpx                mmpx (only used with RenderScale above 1)
//   HDPack     = none | <file or folder>    D2OpenGL.hd next to the DLL, else the folder D2OpenGL-hd
//                                           there (every .hd in it); a relative path is relative to
//                                           the DLL's folder
//   ToggleKey  = none | <letter> | F1..F12  G: switches between upscaled and original sprites
//   [CRT]
//   Scanlines  = 0..1                       0.30
//   Mask       = 0..1                       0.15
//   Glow       = 0..1                       0.08

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

#include "../../common/log.h"

PresentConfig g_presentConfig = {SCALING_AUTO, 0, false, FILTER_SHARP, SHADER_NONE, 2, UPSCALE_MMPX, 0.30f, 0.15f, 0.08f, "", 'G'};

namespace {

void ReadValue(const char *szIni, const char *szSection, const char *szKey, char *szValue, DWORD nSize)
{
    GetPrivateProfileStringA(szSection, szKey, "", szValue, nSize, szIni);
    // Trailing comments and blanks.
    char *pComment = strpbrk(szValue, ";#");
    if (pComment)
        *pComment = '\0';
    size_t n = strlen(szValue);
    while (n > 0 && (szValue[n - 1] == ' ' || szValue[n - 1] == '\t'))
        szValue[--n] = '\0';
}

void ReadFraction(const char *szIni, const char *szKey, float *pfValue)
{
    char szValue[64];
    ReadValue(szIni, "CRT", szKey, szValue, sizeof(szValue));
    if (szValue[0] == '\0')
        return;
    char *pEnd;
    double fValue = strtod(szValue, &pEnd);
    if (pEnd == szValue || *pEnd != '\0' || fValue < 0.0 || fValue > 1.0) {
        d2log("present: [CRT] %s=%s ignored (0 to 1)", szKey, szValue);
        return;
    }
    *pfValue = (float)fValue;
}

void LogUnknownValue(const char *szKey, const char *szValue)
{
    d2log("present: [Display] %s=%s is not a known value, default used", szKey, szValue);
}

// szDir (ending in a backslash) + szName into szOut; false when it does not fit.
bool JoinPath(char *szOut, const char *szDir, size_t nDir, const char *szName)
{
    size_t nName = strlen(szName);
    if (nDir + nName + 1 > MAX_PATH)
        return false;
    memcpy(szOut, szDir, nDir);
    memcpy(szOut + nDir, szName, nName + 1);
    return true;
}

bool FileExists(const char *szPath)
{
    DWORD nAttributes = GetFileAttributesA(szPath);
    return nAttributes != INVALID_FILE_ATTRIBUTES && !(nAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool FolderExists(const char *szPath)
{
    DWORD nAttributes = GetFileAttributesA(szPath);
    return nAttributes != INVALID_FILE_ATTRIBUTES && (nAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

}  // namespace

void Present_LoadConfig(HINSTANCE hSelf)
{
    char szDir[MAX_PATH];
    DWORD n = GetModuleFileNameA(hSelf, szDir, MAX_PATH);
    char *pSlash = n > 0 && n < MAX_PATH ? strrchr(szDir, '\\') : NULL;
    if (pSlash == NULL)
        return;
    size_t nDir = (size_t)(pSlash + 1 - szDir);

    PresentConfig &cfg = g_presentConfig;
    // The default pack: D2OpenGL.hd beside the DLL, else the folder D2OpenGL-hd there.
    if (!(JoinPath(cfg.szHDPack, szDir, nDir, "D2OpenGL.hd") && FileExists(cfg.szHDPack)) &&
        !(JoinPath(cfg.szHDPack, szDir, nDir, "D2OpenGL-hd") && FolderExists(cfg.szHDPack)))
        cfg.szHDPack[0] = '\0';

    char szIni[MAX_PATH];
    if (!JoinPath(szIni, szDir, nDir, "D2OpenGL.ini") || !FileExists(szIni))
        return;
    char szValue[64];

    ReadValue(szIni, "Display", "Scaling", szValue, sizeof(szValue));
    if (_stricmp(szValue, "off") == 0) {
        cfg.eScaling = SCALING_OFF;
    } else if (_stricmp(szValue, "auto") == 0) {
        cfg.eScaling = SCALING_AUTO;
    } else if (szValue[0] != '\0') {
        char *pEnd;
        long nScale = strtol(szValue, &pEnd, 10);
        if (*pEnd == '\0' && nScale >= 1 && nScale <= 16) {
            cfg.eScaling = SCALING_FIXED;
            cfg.nScale = (int)nScale;
        } else {
            LogUnknownValue("Scaling", szValue);
        }
    }

    ReadValue(szIni, "Display", "Fullscreen", szValue, sizeof(szValue));
    if (_stricmp(szValue, "borderless") == 0)
        cfg.bBorderless = true;
    else if (_stricmp(szValue, "exclusive") == 0)
        cfg.bBorderless = false;
    else if (szValue[0] != '\0')
        LogUnknownValue("Fullscreen", szValue);

    ReadValue(szIni, "Display", "Filter", szValue, sizeof(szValue));
    if (_stricmp(szValue, "nearest") == 0)
        cfg.eFilter = FILTER_NEAREST;
    else if (_stricmp(szValue, "sharp") == 0)
        cfg.eFilter = FILTER_SHARP;
    else if (_stricmp(szValue, "linear") == 0)
        cfg.eFilter = FILTER_LINEAR;
    else if (szValue[0] != '\0')
        LogUnknownValue("Filter", szValue);

    ReadValue(szIni, "Display", "Shader", szValue, sizeof(szValue));
    if (_stricmp(szValue, "none") == 0)
        cfg.eShader = SHADER_NONE;
    else if (_stricmp(szValue, "crt") == 0)
        cfg.eShader = SHADER_CRT;
    else if (szValue[0] != '\0')
        LogUnknownValue("Shader", szValue);

    ReadValue(szIni, "Display", "RenderScale", szValue, sizeof(szValue));
    if (szValue[0] != '\0') {
        if (szValue[1] == '\0' && szValue[0] >= '1' && szValue[0] <= '4')
            cfg.nRenderScale = szValue[0] - '0';
        else
            LogUnknownValue("RenderScale", szValue);
    }

    ReadValue(szIni, "Display", "Upscale", szValue, sizeof(szValue));
    if (_stricmp(szValue, "none") == 0)
        cfg.eUpscale = UPSCALE_NONE;
    else if (_stricmp(szValue, "mmpx") == 0)
        cfg.eUpscale = UPSCALE_MMPX;
    else if (szValue[0] != '\0')
        LogUnknownValue("Upscale", szValue);

    char szKey[16];
    ReadValue(szIni, "Display", "ToggleKey", szKey, sizeof(szKey));
    if (_stricmp(szKey, "none") == 0)
        cfg.nToggleKey = 0;
    else if (szKey[0] != '\0' && szKey[1] == '\0' && isalnum((unsigned char)szKey[0]))
        cfg.nToggleKey = toupper((unsigned char)szKey[0]);
    else if ((szKey[0] == 'F' || szKey[0] == 'f') && atoi(szKey + 1) >= 1 && atoi(szKey + 1) <= 12)
        cfg.nToggleKey = VK_F1 + atoi(szKey + 1) - 1;

    char szPack[MAX_PATH];
    ReadValue(szIni, "Display", "HDPack", szPack, sizeof(szPack));
    if (_stricmp(szPack, "none") == 0) {
        cfg.szHDPack[0] = '\0';
    } else if (szPack[0] != '\0') {
        bool bAbsolute = szPack[0] == '\\' || szPack[0] == '/' || szPack[1] == ':';
        bool bFits = bAbsolute ? JoinPath(cfg.szHDPack, "", 0, szPack) : JoinPath(cfg.szHDPack, szDir, nDir, szPack);
        if (!bFits) {
            cfg.szHDPack[0] = '\0';
            d2log("present: [Display] HDPack path is too long");
        }
    }

    // Without the off-screen buffer there is nothing to render at a higher resolution.
    if (cfg.eScaling == SCALING_OFF)
        cfg.nRenderScale = 1;

    ReadFraction(szIni, "Scanlines", &cfg.fScanlines);
    ReadFraction(szIni, "Mask", &cfg.fMask);
    ReadFraction(szIni, "Glow", &cfg.fGlow);
}
