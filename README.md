# Omikron: The Nomad Soul — Linux/Proton Patch

A small Windows DLL that lets **Omikron: The Nomad Soul** (1999, Eidos /
Quantic Dream) run under Linux via Wine or Proton.

[![Build patch.dll](https://github.com/Usires/omikron-proton-patch/actions/workflows/build.yml/badge.svg)](https://github.com/Usires/omikron-proton-patch/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/Usires/omikron-proton-patch)](https://github.com/Usires/omikron-proton-patch/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

> **Want the ready-to-use DLL?** Download the latest
> [`patch.dll`](https://github.com/Usires/omikron-proton-patch/releases/latest)
> from the Releases page — drop it next to `Runtime.exe` in your Omikron
> install directory.

## What it does

The game's `Runtime.exe` imports `DirectDrawCreate` and `DirectDrawEnumerateA`
through a small indirection layer that lets the DirectDraw provider be
swapped without modifying the 1999 binary.

Both **GOG (2017)** and **Steam** releases use the same `Runtime.exe`
(verified by MD5: `02867565a7cf5775d7bc7c905aab9250`). The indirection was
already part of the binary shipped by Quantic Dream / Eidos, not added
later by GOG. Confirmed by `objdump -p`:

```
DLL Name: PATCH.dll
  531ef2   9  DirectDrawEnumerateA
  531ede   7  DirectDrawCreate
```

This repository ships a replacement `patch.dll` (~5 KB, ~150 lines of C)
that **forwards the two DirectDraw calls to Wine's built-in `ddraw.dll`**
via `LoadLibraryA` + `GetProcAddress`. Drop it next to `Runtime.exe` and
the game thinks it's talking to a real DirectDraw provider; in reality,
Wine's modern DirectDraw stack handles the calls — including the
`dgVoodoo2` `D3DImm.dll` / `DDraw.dll` chain that maps legacy DirectDraw
to Vulkan.

## Why a forwarder instead of reimplementing DirectDraw

Wine's built-in `ddraw.dll` already implements DirectDraw on top of modern
graphics drivers. Reimplementing it from scratch would be pointless and
brittle. A thin forwarder is ~150 lines of C, links against nothing, and
gives us the entire Wine DirectDraw stack — including its DirectInput,
DirectSound, and gstreamer integration — for free.

## Build

```bash
./build.sh
```

Requires `i686-w64-mingw32-gcc` (Debian/Ubuntu:
`apt install gcc-mingw-w64-i686`). The script:

1. Compiles `src/patch.c` → `patch.dll`
2. Validates the binary: section VMAs, UCRT absence, export directory
3. Backs up the original `patch.dll` in your game directory as
   `patch.dll.original` (if not already backed up)
4. Copies the new `patch.dll` next to `Runtime.exe`

Default install location is `~/Games/Omikron/`. Edit the `GAME_DIR`
variable in `build.sh` if yours differs.

## Install

Both releases share the same `patch.dll`-based indirection, so the install
procedure is identical — just drop the compiled `patch.dll` next to
`Runtime.exe`:

- **GOG release:** typically `~/Games/Omikron/`
- **Steam release:** typically `~/.steam/steamapps/common/Omikron/`
  (or wherever your Steam library lives, e.g.
  `~/path/to/SteamLibrary/steamapps/common/Omikron/`)

```bash
# Backup the original first (only if no backup exists yet):
cd /path/to/Omikron
[ -f patch.dll.original ] || mv patch.dll patch.dll.original

# Drop in the compiled forwarder:
cp /path/to/repo/patch.dll .
```

Then launch the game. Recommended Steam launch options for the Steam
release (also work for GOG if you launch via Steam with a non-Steam
shortcut):

```
PROTON_LOG=1 %command%
```

Set the Steam Play compatibility tool to **Proton-GE 8-32** (most stable
for this game) if you hit issues with other Proton versions.

You should reach the main menu and play the 3D portions of the game.

## Known Issues

### In-game FMV sequences render as colored pixel noise

The intro and in-game cutscene videos (`EIDOS.AVI.MPG`, `EIDOS.PS.MPG`)
decode to garbled pixels instead of the actual frames. Root cause:

- Wine's 32-bit `winegstreamer` needs 32-bit GStreamer plugins to decode MPG.
- Proton only ships 64-bit GStreamer plugins (`lib64/gstreamer-1.0/*.so`).
- Result: 100+ `wrong ELF class` warnings, plus repeated
  `err:quartz:send_buffer Failed to get a sample, hr 0x80040211` failures.
- The MPG decoder cannot load in the 32-bit Wine process, so frames never
  reach the renderer; the visible "video" is the decoder's default buffer
  or static noise.

Possible fixes tracked for a follow-up release:

- Install 32-bit GStreamer (`lib32-gst-plugins-base`,
  `lib32-gst-plugins-good`) and point `GST_PLUGIN_PATH` at it.
- Transcode the FMVs to a codec Wine's built-in `winegstreamer` can decode
  out of the box (note: the 32-bit plugin gap affects every codec, not just
  MPG — transcoding alone won't fix it unless the 32-bit plugins are also
  present).
- Launch with the `-noffmv` switch to skip the FMVs entirely and play the
  3D-only version of the game.

## Verified Proton versions

| Proton version          | Result                                                      |
|-------------------------|-------------------------------------------------------------|
| Proton-GE 8-32          | ✅ Reaches main menu, 3D gameplay works                     |
| Proton-GE 9-4           | ❌ "Unable to initialize screen" (DirectInput bug)           |
| Proton-GE 11.1          | ✅ Reaches main menu; FMVs broken (see above)               |
| Proton-CachyOS 11.0     | ❌ GStreamer crash (32/64-bit plugin mismatch)              |

Recommended: **Proton-GE 8-32** until the GStreamer / FMV issue is resolved.

## See also

- [`MAKINGOF.md`](MAKINGOF.md) — full reverse-engineering notes, dgVoodoo
  setup, Proton test log, and Wine internals walkthrough.
- [`docs/CI.md`](docs/CI.md) — how the GitHub Actions build and release
  pipelines work, and how to ship your own release.

## Credits

- **Forwarder pattern + build script + Linux port**: Dirk Steiger & Nix
- **Wine DirectDraw stack**: The Wine Project
- **Proton / Proton-GE**: Valve & GloriousEggroll
- **dgVoodoo2**: Dege
- **DSOAL**: Open-source DirectSound3D wrapper
- **Game**: *Omikron: The Nomad Soul* — Quantic Dream, 1999. Please support
  the developers by purchasing a legitimate copy.

By Nix & Dirk, 2026. "Sometimes the cleanest patch is the one that doesn't
patch anything at all."
