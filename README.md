# D2OpenGL

An OpenGL renderer for Diablo II: Lord of Destruction 1.14d on Windows.

Blizzard shipped an OpenGL renderer for Diablo II only on the Mac. The Windows 1.14d game still has
an `-opengl` switch and a slot for that renderer, but nothing behind them. D2OpenGL is the Mac renderer's
code, recovered from the Mac 1.14d binary and rebuilt as a Windows DLL, with a new Windows (WGL) layer
where the Mac code used Apple's APIs.

It is a restoration, not Blizzard's source code. It is not affiliated with or endorsed by Blizzard
Entertainment. You need your own copy of the game.

## Use

Put `D2OpenGL.dll` from the [releases](../../releases) next to the 1.14d `Game.exe` and start the game
through it:

    C:\Windows\SysWOW64\rundll32.exe "C:\path\to\Diablo II\D2OpenGL.dll",Play

Add `-w` after `Play` for a window; other Diablo II options are passed on as well. On 32-bit Windows use
`C:\Windows\System32\rundll32.exe`.

`Play` starts `Game.exe -opengl` with the DLL loaded. `Game.exe` is not modified on disk; the DLL
re-enables the game's OpenGL path in memory. A log is written to `D2OpenGL.log` in the game folder. Any
game version other than 1.14d is left alone.

## Status

- Diablo II 1.14d only.
- Needs a hardware OpenGL driver.
- So far tested on Windows 11 in a virtual machine (Parallels), windowed and fullscreen.

## Build

With the mingw-w64 i686 toolchain (`i686-w64-mingw32-gcc`/`g++`):

    make

The DLL is written to `build/D2OpenGL.dll`.

## Licence

MIT for the code written for this project; the recovered renderer code is not licensed by it. See
[LICENSE](LICENSE).
