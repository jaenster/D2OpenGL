// Shared runtime pieces of the recovered code: the halt idiom and Storm's operator new/delete.

#include "renderer.h"

#include "../../common/log.h"

__attribute__((noinline)) uintptr_t OGL_GetReturnAddress(void)
{
    return reinterpret_cast<uintptr_t>(__builtin_return_address(0));
}

void OGL_Halt(uintptr_t nAddress, int nLine)
{
    d2log("halt: %08lx line %d", (unsigned long)nAddress, nLine);
    if (g_host.Halt)
        g_host.Halt("", nAddress, nLine);
    ExitProcess((UINT)-1);
}

// Mac 000095fc Storm operator new
void *OGL_StormNew(size_t nSize)
{
    return g_host.SMemAlloc(nSize, g_szFileStorm, 35, 0);
}

// Mac 000096b0 Storm operator new[]
void *OGL_StormNewArray(size_t nSize)
{
    return g_host.SMemAlloc(nSize, g_szFileStorm, 50, 0);
}

// Mac 00009632 Storm operator delete
void OGL_StormDelete(void *p)
{
    if (p)
        g_host.SMemFree(p, g_szFileStorm, 40, 0);
}

// Mac 00009671 Storm operator delete[]
void OGL_StormDeleteArray(void *p)
{
    if (p)
        g_host.SMemFree(p, g_szFileStorm, 46, 0);
}

void *CD2Textures::operator new(size_t nSize)
{
    return OGL_StormNew(nSize);
}

void CD2Textures::operator delete(void *p)
{
    OGL_StormDelete(p);
}
