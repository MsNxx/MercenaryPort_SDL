# MercenaryPort

An educational C++ port of *Mercenary*, reconstructed from the Atari ST version.
Original game by Paul Woakes and Novagen.
Have attempted to remain faithful to original architecture as much as possible.
Provenance address scattered about in comments may not match all copies of the original.
See also Mercenary Collection at https://mercenary.kiigan.com

See DevUtils.h for handy overrides etc.

- MsNxx

## Requirements

- CMake 3.20+
- A C++20 compiler
- SDL2 (fetched automatically if not installed system-wide)

## Building

**macOS / Linux**

    scripts/build-release.sh
    -or-
    scripts/build-debug.sh

**Windows**

    scripts\build-release.bat
    -or-
    scripts\build-debug.bat

The resulting binary is placed in `_build/`.

## License

MIT. See [LICENSE](LICENSE).
