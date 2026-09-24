#ifndef D2OPENGL_LOG_H
#define D2OPENGL_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Appends one line to D2OpenGL.log in the working directory. */
void d2log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void d2log_reset(void);

#ifdef __cplusplus
}
#endif

#endif
