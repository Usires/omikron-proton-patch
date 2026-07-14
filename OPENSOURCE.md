# OPENSOURCE

This project stands on the shoulders of giants. Here's who built what we
depend on, and the licenses they did it under.

## Direct dependencies

### Wine
- **What we use:** Wine's builtin `ddraw.dll` implementation of
  `DirectDrawCreate` and `DirectDrawEnumerateA`. Loaded at runtime via
  `LoadLibraryA("ddraw.dll")` + `GetProcAddress`.
- **License:** GNU Lesser General Public License v2.1+
- **Why we love it:** Wine's DirectDraw stack handles 25 years of
  legacy DirectDraw games competently. Reimplementing it would be
  hundreds of thousands of lines of code we don't want to write.
- **Link:** https://www.winehq.org/

### dgVoodoo2
- **What we use:** D3D→Vulkan translation layer (specifically the
  `D3DImm.dll` and `DDraw.dll` components) that lets legacy DirectDraw
  games run on Vulkan-capable GPUs.
- **License:** Free for personal use (proprietary, freeware)
- **Author:** Dege
- **Link:** http://dege.freeweb.hu/

### DSOAL
- **What we use:** `dsound.dll` and `dsoal-aldrv.dll` providing a
  DirectSound3D wrapper layer that fixes audio positioning issues in
  legacy DirectSound-using games.
- **License:** GNU Lesser General Public License v2.1
- **Link:** https://github.com/kcat/dsoal

### Proton / Proton-GE
- **What we use:** Valve's Proton compatibility layer, plus GloriousEggroll's
  community-maintained Proton-GE builds with extra patches for older games.
- **License:** Various open-source (mostly BSD, MIT, GPL)
- **Link:** https://github.com/ValveSoftware/Proton
- **Link (GE):** https://github.com/GloriousEggroll/proton-ge-custom

### MinGW-w64 (i686-w64-mingw32-gcc)
- **What we use:** Cross-compiler toolchain targeting 32-bit Windows from
  Linux, used to build the forwarder DLL.
- **License:** Various (mostly GPL for GCC runtime, BSD/MIT for
  toolchain components)
- **Link:** https://www.mingw-w64.org/

## Inspirations and references

- **Wine's `dinput` and `ddraw` source code**: read extensively while
  designing the forwarder pattern. The lazy `LoadLibraryA` + per-call
  `GetProcAddress` pattern is idiomatic Wine.
- **ReactOS' DirectDraw implementation**: cross-referenced for export
  table layouts and `DDENUMCALLBACKA` calling conventions.

## Game credits

*Omikron: The Nomad Soul* — Quantic Dream, 1999.
Designed by **David Cage**. Published by **Eidos Interactive**.

The game's original developers and artists are not affiliated with this
project. Please support them by purchasing a legitimate copy.

## License of this project

This project itself is released under the **MIT License**. See
[`LICENSE`](LICENSE) for the full text.

By Nix & Dirk, 2026. "Standing on the shoulders of giants, one
`LoadLibraryA` at a time."
