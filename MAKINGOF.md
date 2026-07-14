# MAKINGOF — Omikron Linux/Proton Patch

The full story of how this 150-line forwarder came to be.

## The Goal

Run *Omikron: The Nomad Soul* (1999, Quantic Dream) on Linux under Proton
without rewriting DirectDraw from scratch. The game is a Windows-only 32-bit
title with no native Linux port and no surviving official patch pipeline —
the kind of game that needs a small, surgical intervention to keep playable
on modern systems.

## The Problem

`Runtime.exe` imports two functions from `ddraw.dll`:

- `DirectDrawCreate`
- `DirectDrawEnumerateA`

In a clean Wine prefix, those imports resolve to Wine's builtin `ddraw.dll`
and the game launches. On Proton, however, the resolution depends on which
Proton version's `ddraw.dll` ends up `WINEDLLOVERRIDES`-shadowing Wine's,
and on whether Steam has placed a different DirectDraw shim ahead of it
(`ddraw.dll` from `dgVoodoo2`, `d8d9`, `d3d9`, etc.).

Some Proton versions work. Some don't. The game also crashes on startup in
certain versions because of a `DIERR_DEVICENOTREG` (DirectInput device not
registered) bug specific to those Proton builds. There is no single
Proton version that "just works."

## The Insight

We don't actually need to *fix* DirectDraw. We need the game to call into
*something* that returns a DirectDraw interface it can use — and Wine's
builtin `ddraw.dll` already does that, layered on top of `dgVoodoo2`'s
D3D→Vulkan shim. The game doesn't care which DLL answers the call, as
long as `DirectDrawCreate` returns a working `IDirectDraw*`.

A forwarder DLL — one that re-exports the two functions and `LoadLibraryA`s
Wine's `ddraw.dll` to resolve them — gives us:

1. A stable, version-independent entry point that always points at the
   correct `ddraw.dll` (Wine's builtin) at runtime.
2. Full compatibility with whatever Proton/Wine/Steam have layered on top
   of the DirectDraw stack (dgVoodoo2, DSOAL, custom D3D wrappers).
3. Zero maintenance: if Wine improves DirectDraw, we improve automatically.

## The Forwarder Pattern

```c
static HMODULE g_ddraw = NULL;

static HMODULE get_ddraw(void) {
    if (g_ddraw) return g_ddraw;
    g_ddraw = LoadLibraryA("ddraw.dll");
    return g_ddraw;
}

HRESULT WINAPI DirectDrawCreate(
    GUID *lpGUID, LPVOID *lplpDD, IUnknown *pUnkOuter)
{
    HMODULE h = get_ddraw();
    if (!h) return E_FAIL;
    PFN_DirectDrawCreate fn =
        (PFN_DirectDrawCreate)GetProcAddress(h, "DirectDrawCreate");
    if (!fn) return E_FAIL;
    return fn(lpGUID, lplpDD, pUnkOuter);
}
```

Lazy `LoadLibraryA` so we never pay the cost of resolution if the game
never calls the function. `GetProcAddress` cached per-call (Wine's own
internal cache would just re-look these up anyway, but this keeps the
contract obvious).

## Build Constraints

A few non-obvious requirements surfaced while iterating on the build:

- **Image base `0x10000000`**: keeps the DLL out of the low-address range
  Wine reserves for builtin DLLs. Loading our DLL at a Wine-builtin address
  triggers subtle loader path issues with `WINEDLLOVERRIDES`.
- **No UCRT imports**: linking against the Universal C Runtime (`-lucrt`,
  `-lucrtbase`) drags in `api-ms-win-crt-*.dll` dependencies that some
  Proton versions don't ship. The forwarder doesn't need any C runtime —
  `LoadLibraryA` and `GetProcAddress` are in `kernel32.dll`, which is
  always present. The build script verifies with `objdump -p | grep
  api-ms-win-crt` and aborts if found.
- **`-Wl,--kill-at`**: drops the `@12` / `@8` stdcall decoration suffixes
  from exports, so the function names match what `Runtime.exe` actually
  imports.
- **`-Wl,-u,_DirectDrawCreate@12` / `-Wl,-u,_DirectDrawEnumerateA@8`**: the
  C source file never references the unprefixed symbols, so the linker
  would strip them as unused. `-u` forces them to be exported.
- **`-Wl,--exclude-libs=ALL`**: prevents any implicit symbol exports from
  GCC's runtime support libraries leaking into our export table.

## Proton Test Log

Each row is a real launch attempt with the forwarder DLL in place:

| Date       | Time   | Proton version       | Result                              | Notes                                              |
|------------|--------|----------------------|-------------------------------------|----------------------------------------------------|
| 2026-07-13 | 17:50  | Proton-GE 8-32       | ✅ Reaches main menu                | Videos broken, then crash                          |
| 2026-07-13 | 18:41  | Proton-GE 11.1       | ❌ "Impossible to initialize screen" | patch.dll disabled via WineCfg                     |
| 2026-07-13 | 18:46  | Proton-GE 11.1       | ✅ In game, then crash after videos  | patch.dll re-enabled                               |
| 2026-07-13 | 18:51  | Proton-GE 8-32       | ✅ Main menu, videos broken          | —                                                  |
| 2026-07-13 | 19:43  | Proton-GE 9-4        | ❌ "Unable to initialize screen"   | DirectInput error `DIERR_DEVICENOTREG`             |
| 2026-07-13 | 19:51  | Proton-CachyOS 11.0  | ❌ GStreamer crash                  | 32/64-bit plugin mismatch, immediate crash         |

Current recommendation: **Proton-GE 8-32** until the GStreamer / FMV
issue is fixed.

## What We Tried That Didn't Work

- **Dummy `patch.dll` returning `E_FAIL` from `DirectDrawCreate`**:
  The game detects the failure and bails with "Unable to initialize
  screen." Useful as a control test — proves the patch is being loaded.
- **`WINEDLLOVERRIDES=ddraw=d`** (force builtin): identical behavior to
  no override. Wine's builtin was already being used.
- **`mfplat=d`, `quartz=d`**: testing if forcing builtin Media Foundation
  or quartz would help the FMV decode. No effect — the videos are still
  broken, but the 3D portions render correctly regardless.
- **dgVoodoo2 `ColorSpace=rgb` + `Default3DRenderFormat=argb8888`**: This
  was a guess at the FMV issue. It is the correct setting for color
  accuracy in 3D rendering, but does not affect the gstreamer-based video
  decode path. The colored pixels were not a color-space problem.

## Architecture Notes

The full runtime call chain when the game is running:

```
Runtime.exe
  └─ patch.dll (this repo, ~150 lines)
       └─ LoadLibraryA("ddraw.dll")  ←  Wine's builtin
            └─ DirectDrawCreate → IDirectDraw7
                 └─ dgVoodoo2 (D3DImm.dll, DDraw.dll)
                      └─ Vulkan
                           └─ GPU
```

The forwarder adds exactly one extra dynamic-link step at `DirectDrawCreate`
time. After that, the call graph is identical to a native Wine install.

## Future Work

1. **FMV fix**: install 32-bit GStreamer plugins (`lib32-gst-plugins-base`,
   `lib32-gst-plugins-good`) on the host. Update Proton prefix with
   `GST_PLUGIN_PATH=/usr/lib32/gstreamer-1.0`. Re-test.
2. **CI**: GitHub Actions matrix across Proton-GE 7-x, 8-x, 9-x, 10-x,
   11-x launching the game in headless mode and asserting main-menu
   reachability. Requires `xvfb` and a captured screenshot of the main
   menu for diff comparison.
3. **Steam launch options documentation**: a Steam shortcut that sets
   `PROTON_USE_WINED3D=1`, `GST_PLUGIN_PATH=/usr/lib32/gstreamer-1.0`,
   and `WINEDEBUG=-quartz` for FMV diagnostics.

## Credits

- **Forwarder pattern, build script, and Linux port**: Dirk Steiger & Nix
- **Wine DirectDraw stack**: The Wine Project contributors
- **Proton-GE**: GloriousEggroll
- **dgVoodoo2**: Dege
- **DSOAL**: Open-source DirectSound3D wrapper community
- **Game**: *Omikron: The Nomad Soul* — Quantic Dream, 1999

By Nix & Dirk, 2026. "The shortest path between a 1999 DirectDraw call and
a 2026 GPU is sometimes just a `LoadLibraryA` away."
