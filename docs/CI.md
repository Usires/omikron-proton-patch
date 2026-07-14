# Continuous Integration

This repository uses GitHub Actions for automated builds.

## What runs on every push / PR

The [Build patch.dll](../.github/workflows/build.yml) workflow:

1. Spins up an Ubuntu runner.
2. Installs `gcc-mingw-w64-i686` and `binutils-mingw-w64-i686`.
3. Runs `./build.sh` to compile `src/patch.c` → `patch.dll`.
4. Validates the resulting binary:
   - Confirms only `kernel32.dll` is imported (no UCRT).
   - Confirms `DirectDrawCreate` and `DirectDrawEnumerateA` are exported.
   - Reports file size and ELF headers.
5. Uploads `patch.dll` as a build artifact (downloadable from the run page).

This gives you a green check on every commit, so reviewers can see the
binary builds before approving a PR — even if they don't have the
MinGW toolchain installed locally.

## What runs on a version tag

The [Release](../.github/workflows/release.yml) workflow triggers when you
push a tag matching `v*` (e.g. `v0.1.0`, `v1.0.0`, `v2.0.0-rc1`).

It does everything the build workflow does, plus:

1. Extracts the matching section from `CHANGELOG.md` to use as release
   notes (falls back to a generic message if no section is found).
2. Creates a GitHub Release named after the tag.
3. Attaches the freshly-built `patch.dll` as a downloadable asset.

### Creating a new release

From your local clone:

```bash
# Make sure CHANGELOG.md has a section for the new version.
# Then commit, tag, and push:
git add CHANGELOG.md
git commit -m "Release v0.2.0"
git tag v0.2.0
git push origin main
git push origin v0.2.0
```

The Actions runner picks up the tag, builds the DLL, and publishes the
release within ~2 minutes.

### Pre-release tags

Tags containing a hyphen (e.g. `v0.2.0-rc1`, `v1.0.0-beta`) are
automatically marked as **pre-releases** on GitHub. They still appear in
the Releases list, but with a "Pre-release" badge and don't show up in
the "Latest" badge.

## Permissions

The release workflow needs `contents: write` permission to create releases.
This is configured explicitly in the workflow file — the repo's default
token permissions remain unchanged for everything else.

## Local parity

To replicate the CI build locally on a Debian/Ubuntu host:

```bash
sudo apt install gcc-mingw-w64-i686 binutils-mingw-w64-i686
./build.sh
```

The `build.sh` script is the single source of truth for how the DLL is
built — if you ever need to change compiler flags, change them there, not
in the CI workflow.

By Nix & Dirk, 2026. "If it's not in CI, it's not real."
