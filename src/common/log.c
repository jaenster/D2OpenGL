#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#include "log.h"

#define LOG_NAME "D2OpenGL.log"

void d2log_reset(void)
{
    DeleteFileA(LOG_NAME);
}

void d2log(const char *fmt, ...)
{
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line - 2, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (n > (int)sizeof line - 3)
        n = sizeof line - 3;
    line[n++] = '\r';
    line[n++] = '\n';
    HANDLE f = CreateFileA(LOG_NAME, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return;
    DWORD written;
    WriteFile(f, line, (DWORD)n, &written, NULL);
    CloseHandle(f);
}
