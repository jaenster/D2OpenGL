#ifndef D2OPENGL_RENDERER_H
#define D2OPENGL_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

#define RENDERER_SLOT_COUNT 54

/* Binds g_host for the running build. FALSE if the build is unknown or a binding is missing. */
int renderer_host_init(void);

/* The 54-slot Windows table of the restored renderer (renderer/slots.cpp). */
void *const *renderer_table(void);

#ifdef __cplusplus
}
#endif

#endif
