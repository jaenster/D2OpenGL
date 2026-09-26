# D2OpenGL

An OpenGL renderer for Diablo II: Lord of Destruction on Windows (1.14d, 1.13d, 1.13c, 1.10f).

Blizzard shipped an OpenGL renderer for Diablo II only on the Mac. The Windows game still has an
`-opengl` switch and a slot for that renderer, but nothing behind them. D2OpenGL is the Mac renderer's
code, recovered from the Mac 1.14d binary and rebuilt as a Windows DLL, with a new Windows (WGL) layer
where the Mac code used Apple's APIs.

It is a restoration, not Blizzard's source code. It is not affiliated with or endorsed by Blizzard
Entertainment. You need your own copy of the game.

## Use

Put `D2OpenGL.dll` from the [releases](../../releases) in the game folder.

**1.14d, 1.13d, 1.13c:** start the game through the DLL:

    C:\Windows\SysWOW64\rundll32.exe "C:\path\to\Diablo II\D2OpenGL.dll",Play

Add `-w` after `Play` for a window; other Diablo II options are passed on as well. On 32-bit Windows use
`C:\Windows\System32\rundll32.exe`. `Play` starts `Game.exe -opengl` with the DLL loaded.

**1.10f:** the original `Game.exe` does not run on current Windows. With D2Loader, put `D2OpenGL.dll` in
its `plugin` folder instead and start D2Loader with `-opengl`. Windows 10 and 11 show a compatibility
warning for the 1.10 files; the game runs regardless.

No game file is modified on disk; the DLL re-enables the game's OpenGL path in memory, and only with
`-opengl`. A log is written to `D2OpenGL.log` in the game folder. Other game versions are left alone.

## Display

Settings are read from `D2OpenGL.ini` next to `D2OpenGL.dll`; without the file the defaults apply.

    [Display]
    Scaling    = auto        ; auto, off, or a whole number
    Fullscreen = exclusive   ; exclusive or borderless
    Filter     = sharp       ; sharp, nearest or linear
    Shader     = none        ; none or crt
    RenderScale = 2          ; 1 to 4
    Upscale    = mmpx        ; mmpx or none

    [CRT]
    Scanlines  = 0.30        ; 0 to 1
    Mask       = 0.15
    Glow       = 0.08

- **Scaling**: the game still runs at 640x480 or 800x600; the picture is scaled up to the window.
  `auto` windowed picks the largest whole multiple that fits the screen. `off` draws exactly as the Mac
  renderer does, with no scaling and a display mode switch in fullscreen.
- **Fullscreen**: `exclusive` switches the display to the game's resolution; `borderless` covers the
  monitor at the desktop resolution and fits the picture to it, keeping the aspect ratio (still has
  problems).
- **Filter**: `sharp` keeps pixels square and crisp at any size, `nearest` repeats pixels, `linear`
  blurs.
- **Shader**: `crt` imitates a late-1990s PC monitor (aperture grille, fine scanlines, a little glow).
- **RenderScale**: the picture is drawn at this multiple of the game's resolution before it is scaled
  to the window. `1` draws at the game's resolution. Ignored with `Scaling = off`.
- **Upscale**: with a render scale above 1, `mmpx` enlarges sprites (characters, monsters, items, the
  interface) with the MMPX pixel-art filter before they are coloured, so they keep the game's 256
  colours and colour variants; `none` draws them with plain square pixels. Floors and walls are not
  enlarged. At `RenderScale = 3` sprites are enlarged 2x and drawn at 3x.

## Status

- Diablo II 1.14d, 1.13d, 1.13c and 1.10f.
- So far tested on Windows 11 in a virtual machine (Parallels). 1.14d windowed and fullscreen; 1.13d and
  1.13c in game, windowed; 1.10f to the main menu, windowed and fullscreen. Scaling and the CRT shader
  so far only on 1.14d.

## Build

With the mingw-w64 i686 toolchain (`i686-w64-mingw32-gcc`/`g++`) and Zig 0.16 (for the libd2
submodule):

    git submodule update --init
    make

The DLL is written to `build/D2OpenGL.dll`.

## Licence

MIT for the code written for this project; the recovered renderer code is not licensed by it. See
[LICENSE](LICENSE).
