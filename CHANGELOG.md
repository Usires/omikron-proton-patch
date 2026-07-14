# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- README and MAKINGOF: corrected the assumption that the Steam release
  lacked the `patch.dll` indirection. Verified by comparing the Steam
  and GOG `Runtime.exe` binaries via MD5 — they are identical
  (`02867565a7cf5775d7bc7c905aab9250`), and both import
  `DirectDrawCreate` / `DirectDrawEnumerateA` from `PATCH.dll`. The
  indirection is part of the original 1999 binary, not a GOG addition.
- README install section merged: both GOG and Steam use the same
  procedure (drop `patch.dll` next to `Runtime.exe`).

## [0.1.0] - 2026-07-14

### Added
- Initial `patch.dll` forwarder: forwards `DirectDrawCreate` and
  `DirectDrawEnumerateA` to Wine's builtin `ddraw.dll`.
- `build.sh`: cross-compile script with i686-w64-mingw32-gcc, validates
  UCRT absence, exports, and section VMAs.
- `LICENSE` (MIT).
- `README.md` with build instructions, install guide, and known issues.
- `MAKINGOF.md` with full reverse-engineering notes, Proton test log,
  and architectural details.

### Known Issues
- In-game FMVs render as colored pixel noise due to 32-bit GStreamer
  plugin gap in Proton (see README "Known Issues" and MAKINGOF).

[Unreleased]: https://github.com/Usires/omikron-proton-patch/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Usires/omikron-proton-patch/releases/tag/v0.1.0
