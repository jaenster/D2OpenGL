/* Fault logging (crashlog.h). */

#include "crashlog.h"

#include "../common/log.h"

static HMODULE s_self;
static DWORD_PTR s_base;
static DWORD_PTR s_end;
static LONG s_count;

/* "name+0xoffset" for the module holding addr, or the bare address. */
static void describe(DWORD_PTR addr, char *out, int size)
{
    HMODULE mod = NULL;
    char path[MAX_PATH];
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)addr, &mod) &&
        GetModuleFileNameA(mod, path, MAX_PATH)) {
        const char *name = path;
        for (const char *p = path; *p; p++)
            if (*p == '\\' || *p == '/')
                name = p + 1;
        wsprintfA(out, "%s+0x%lx", name, (unsigned long)(addr - (DWORD_PTR)mod));
    } else {
        wsprintfA(out, "0x%08lx", (unsigned long)addr);
    }
    (void)size;
}

static LONG CALLBACK on_fault(EXCEPTION_POINTERS *info)
{
    DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;
    if (InterlockedIncrement(&s_count) > 3)
        return EXCEPTION_CONTINUE_SEARCH;
    const CONTEXT *ctx = info->ContextRecord;
    char where[MAX_PATH + 32];
    describe((DWORD_PTR)info->ExceptionRecord->ExceptionAddress, where, sizeof(where));
    if (code == EXCEPTION_ACCESS_VIOLATION)
        d2log("crash: access violation at %s, %s 0x%08lx (thread %lu)", where,
              info->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
              (unsigned long)info->ExceptionRecord->ExceptionInformation[1], GetCurrentThreadId());
    else
        d2log("crash: exception 0x%08lx at %s (thread %lu)", code, where, GetCurrentThreadId());
    d2log("crash: eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx", ctx->Eax,
          ctx->Ebx, ctx->Ecx, ctx->Edx, ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp);
    /* Frame pointers are omitted, so scan the stack for values inside this DLL: return addresses, and
     * the odd stale pointer. */
    const DWORD_PTR *sp = (const DWORD_PTR *)ctx->Esp;
    MEMORY_BASIC_INFORMATION mbi;
    int found = 0;
    if (VirtualQuery(sp, &mbi, sizeof(mbi))) {
        const DWORD_PTR *top = (const DWORD_PTR *)((BYTE *)mbi.BaseAddress + mbi.RegionSize);
        for (int i = 0; i < 1024 && sp + i < top && found < 16; i++) {
            DWORD_PTR v = sp[i];
            if (v >= s_base && v < s_end) {
                d2log("crash:   stack+0x%x: D2OpenGL.dll+0x%lx", i * 4, (unsigned long)(v - s_base));
                found++;
            }
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void crashlog_install(HMODULE self)
{
    s_self = self;
    s_base = (DWORD_PTR)self;
    const IMAGE_NT_HEADERS *nt =
        (const IMAGE_NT_HEADERS *)((const BYTE *)self + ((const IMAGE_DOS_HEADER *)self)->e_lfanew);
    s_end = s_base + nt->OptionalHeader.SizeOfImage;
    d2log("D2OpenGL: loaded at 0x%08lx", (unsigned long)s_base);
    AddVectoredExceptionHandler(1, on_fault);
}
