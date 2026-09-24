#include <windows.h>
#include <string.h>

#include "../../common/log.h"
#include "install.h"

BOOL install_patch(DWORD va, const void *expect, const void *bytes, SIZE_T len)
{
    void *at = (void *)(DWORD_PTR)va;
    if (expect && memcmp(at, expect, len) != 0) {
        d2log("install: %08lx holds unexpected bytes, not patched", (unsigned long)va);
        return FALSE;
    }
    DWORD old;
    if (!VirtualProtect(at, len, PAGE_EXECUTE_READWRITE, &old))
        return FALSE;
    memcpy(at, bytes, len);
    VirtualProtect(at, len, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, len);
    return TRUE;
}
