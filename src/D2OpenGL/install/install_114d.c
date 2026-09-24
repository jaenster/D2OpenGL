/* 1.14d Game.exe: everything is linked into the exe, which has no relocations. */
#include <windows.h>

#include "install.h"

/* RENDERER_RenderedFunctionsSelector[5], the OpenGL entry D2gfx leaves NULL. */
#define SELECTOR_OPENGL_SLOT  0x0072DA94u

/* ApplicationMain picks the D2gfx mode from the 3dfx, window and d3d ini bytes only; the
 * opengl byte (+0x0b, which ClientInit_Renderer_OpenGL reads) is never consulted.
 * The detour replaces "cmp byte [esi+0Ah],0 / je 405CD2" at 405CB3 with a jump to
 * select_mode, which checks opengl first and otherwise does what the original did. */
#define MODE_SELECT_SITE      0x00405CB3u

/* WinMain parses [VIDEO] OPENGL / -opengl into the ini struct and then clears it again
 * ("mov byte [ebp-4CDh],0") right before ApplicationMain. That store becomes a 7-byte nop. */
#define OPENGL_CLEAR_SITE     0x00406595u

void *mode_select_resume_3dfx = (void *)0x00405CB9;  /* mov [ebp+8],4 */
void *mode_select_resume_next = (void *)0x00405CD2;  /* the window / d3d tests */
void *mode_select_resume_done = (void *)0x00405CED;  /* mode chosen */

extern const char select_mode[];
__asm__(".globl _select_mode\n"
        "_select_mode:\n"
        "    cmpb $0, 0xb(%esi)\n"
        "    jne  1f\n"
        "    cmpb $0, 0xa(%esi)\n"
        "    je   2f\n"
        "    jmp  *_mode_select_resume_3dfx\n"
        "2:  jmp  *_mode_select_resume_next\n"
        "1:  movl $5, 0x8(%ebp)\n"
        "    jmp  *_mode_select_resume_done\n");

BOOL install_114d(void *const *table)
{
    static const BYTE original[6] = {0x80, 0x7E, 0x0A, 0x00, 0x74, 0x19};
    BYTE jump[6];
    DWORD rel = (DWORD)(DWORD_PTR)select_mode - (MODE_SELECT_SITE + 5);
    jump[0] = 0xE9;
    CopyMemory(jump + 1, &rel, 4);
    jump[5] = 0x90;

    static const BYTE clear_opengl[7] = {0xC6, 0x85, 0x33, 0xFB, 0xFF, 0xFF, 0x00};
    static const BYTE nop7[7] = {0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00};
    static const void *const empty = NULL;
    if (!install_patch(SELECTOR_OPENGL_SLOT, &empty, &table, sizeof table))
        return FALSE;
    if (!install_patch(OPENGL_CLEAR_SITE, clear_opengl, nop7, sizeof nop7))
        return FALSE;
    return install_patch(MODE_SELECT_SITE, original, jump, sizeof jump);
}
