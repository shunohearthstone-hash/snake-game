# Windows build artifacts

This folder contains the Windows executable build of the Snake game.

- snake.exe — statically linked with PDCurses (WinCon backend)
- Built with MinGW-w64 gcc from MSYS2

Notes:
- Source headers from PDCurses are not included here.
- We intentionally do not include any `colors.h` from `PDCurses/wincon` (none is required).
- If you rebuild, the root `snake.exe` is copied here as `windows/snake.exe`.
