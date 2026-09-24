#ifndef D2OPENGL_BUILD_H
#define D2OPENGL_BUILD_H

#ifdef __cplusplus
extern "C" {
#endif

/* The game builds this project knows. Everything address-specific is keyed on this. */
typedef enum GameBuildId {
    BUILD_UNKNOWN = 0,
    BUILD_114D,
    BUILD_113D,
    BUILD_113C,
    BUILD_110F,
} GameBuildId;

/* Recognises the running game: 1.14d by its Game.exe, the DLL-era builds by D2gfx.dll. */
GameBuildId build_identify(void);
/* The same, for a game folder on disk. */
GameBuildId build_identify_folder(const char *folder);
const char *build_name(GameBuildId id);

#ifdef __cplusplus
}
#endif

#endif
