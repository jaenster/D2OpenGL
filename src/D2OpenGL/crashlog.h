#ifndef D2OPENGL_CRASHLOG_H
#define D2OPENGL_CRASHLOG_H

#include <windows.h>

/* Logs where the process faults: the module and offset of the faulting instruction, the registers, and
 * the return addresses on the stack that fall inside this DLL, as offsets `nm`/`addr2line` resolve
 * against the built DLL. It only logs (the first few faults) and lets the fault go on as before.
 * Also logs where this DLL is loaded. */
void crashlog_install(HMODULE self);

#endif
