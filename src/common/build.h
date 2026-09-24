#ifndef D2OPENGL_BUILD_H
#define D2OPENGL_BUILD_H

#ifdef __cplusplus
extern "C" {
#endif

/* The game builds this project knows. Everything address-specific is keyed on this. */
typedef enum GameBuildId {
    BUILD_UNKNOWN = 0,
    BUILD_114D,
} GameBuildId;

/* Recognises the running Game.exe by PE timestamp, SizeOfImage and load address. */
GameBuildId build_identify(void);
const char *build_name(GameBuildId id);

#ifdef __cplusplus
}
#endif

#endif
