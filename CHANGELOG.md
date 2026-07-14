# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- README and MAKINGOF: clarified that the patch applies to both GOG and
  Steam releases, with a Steam-specific install section explaining the
  Proton `WINEDLLOVERRIDES` shortcut for the 1999 binary that lacks the
  `patch.dll` indirection layer.

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
