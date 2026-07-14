# Omikron: The Nomad Soul — Linux/Proton Patch

A small Windows DLL that lets **Omikron: The Nomad Soul** (1999, Eidos /
Quantic Dream) run under Linux via Wine or Proton.

## What it does

The game's `Runtime.exe` imports `DirectDrawCreate` and `DirectDrawEnumerateA`
from `ddraw.dll`. This repository ships a custom `ddraw.dll`-compatible DLL
(`patch.dll`) that **forwards those two functions to Wine's built-in
`ddraw.dll`** via `LoadLibraryA` + `GetProcAddress`.

The game thinks it's talking to a real DirectDraw provider; in reality, Wine's
modern DirectDraw stack handles the calls — including the
`dgVoodoo2`/`D3DImm`/`DDraw` chain that maps legacy DirectDraw to Vulkan.

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

After running `build.sh`, launch the game through Steam (Proton) or Wine:

```bash
# Steam (Proton-GE 8-32 recommended for now — see Known Issues)
# Just launch Omikron from your Steam library.

# Wine, manual:
cd ~/Games/Omikron
wine Runtime.exe
```

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
