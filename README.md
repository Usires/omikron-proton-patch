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
swapped without modifying the 1999 binary. Depending on which release you
have, this indirection takes different forms:

- **GOG release (2017):** `Runtime.exe` imports the two functions from a
  file called `patch.dll`, which is itself a tiny DirectDraw shim. We
  replace GOG's `patch.dll` with our forwarder.
- **Steam release (1999 original):** the indirection was added later by
  GOG; the original Steam binary may import directly from `ddraw.dll`.
  Proton's DLL override system (`WINEDLLOVERRIDES`) usually routes this
  to Wine's builtin `ddraw.dll` without our intervention — see the Steam
  install notes below for the edge cases.

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

### GOG release

After running `build.sh`, launch the game through Steam (Proton) or Wine:

```bash
# Steam (Proton-GE 8-32 recommended for now — see Known Issues)
# Just launch Omikron from your Steam library.

# Wine, manual:
cd ~/Games/Omikron
wine Runtime.exe
```

You should reach the main menu and play the 3D portions of the game.

### Steam release

The original 1999 Steam binary has no `patch.dll` indirection. The good
news: Proton's DLL override system (`WINEDLLOVERRIDES=ddraw=n,b`) typically
routes DirectDraw calls to Wine's builtin `ddraw.dll` without us needing
to ship a shim at all. Try just launching Omikron in Steam first.

If the Steam version crashes or fails to initialize the screen (typically
on Proton-GE 9-x with the `DIERR_DEVICENOTREG` DirectInput bug), either:

1. Switch Proton version to **Proton-GE 8-32** (most stable for this game),
   or
2. Install our `patch.dll` as a Steam override:
   - Set Steam Launch Options to:
     `WINEDLLOVERRIDES=ddraw=n,b %command%`
   - Place `patch.dll` from this repo into the Steam Omikron install
     directory (typically `~/.steam/steam/steamapps/common/Omikron/`).

If you go the second route, you'll also need to introduce the indirection
into `Runtime.exe` itself — repack its IAT to import from `patch.dll`
instead of `ddraw.dll`. That's a larger surgery and out of scope for the
initial release; see [MAKINGOF.md](MAKINGOF.md) for the technical
background.

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
