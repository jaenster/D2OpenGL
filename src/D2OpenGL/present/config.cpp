// D2OpenGL.ini next to the DLL, read once. A missing file, key or unknown value gives the default.
//
//   [Display]
//   Scaling    = auto | off | <integer>     auto
//   Fullscreen = exclusive | borderless     exclusive
//   Filter     = sharp | nearest | linear   sharp
//   Shader     = none | crt                 none
//   [CRT]
//   Scanlines  = 0..1                       0.30
//   Mask       = 0..1                       0.15
//   Glow       = 0..1                       0.08

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

#include "../../common/log.h"

PresentConfig g_presentConfig = {SCALING_AUTO, 0, false, FILTER_SHARP, SHADER_NONE, 0.30f, 0.15f, 0.08f};

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

}  // namespace

void Present_LoadConfig(HINSTANCE hSelf)
{
    char szIni[MAX_PATH];
    DWORD n = GetModuleFileNameA(hSelf, szIni, MAX_PATH);
    char *pSlash = n > 0 && n < MAX_PATH ? strrchr(szIni, '\\') : NULL;
    if (pSlash == NULL || (size_t)(pSlash + 1 - szIni) + sizeof("D2OpenGL.ini") > sizeof(szIni))
        return;
    strcpy(pSlash + 1, "D2OpenGL.ini");
    if (GetFileAttributesA(szIni) == INVALID_FILE_ATTRIBUTES)
        return;

    PresentConfig &cfg = g_presentConfig;
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

    ReadFraction(szIni, "Scanlines", &cfg.fScanlines);
    ReadFraction(szIni, "Mask", &cfg.fMask);
    ReadFraction(szIni, "Glow", &cfg.fGlow);
}
