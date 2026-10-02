# AppStream Identity & Packaging Names (WS19) — transient

## Goal

Issue #309: use one standard application ID, `org.socnetv.SocNetV`, for the AppStream metadata
file, desktop file, icon and window/launcher association, correct the metadata license, and leave
the project Flathub-ready (#167) — **without breaking** any existing distribution channel.

Transient workstream: scoped to the 3.9 cycle. Fold the lasting parts (naming convention,
verification matrix) into the release procedure and delete this file once shipped.

## Status

Planned. Nothing implemented. Audit complete; coordination with downstream packagers pending.

## Decisions (made)

- **ID:** `org.socnetv.SocNetV` (replaces `org.socnetv.social_network_visualizer`; the old ID is
  declared as replaced in the metadata — verify the exact element against the AppStream spec).
- **Rename the source files with `git mv`; never use CMake's install `RENAME`.** qmake (used by
  Fedora) cannot rename on install, so both build systems must install files by their own names.
- **All renames at once, in 3.9.** v3.8 and its tarball stay untouched.
- **License field:** `GPL-3.0-or-later` (COPYING is GPLv3; source headers say "version 3 or
  later"). The current `GPL-2.0+` is wrong.
- **Window association:** call `QGuiApplication::setDesktopFileName("org.socnetv.SocNetV")`, so
  the launcher/dock still associates the window with the renamed desktop file.
- **No new installed path categories in upstream.** The icon stays a PNG in `share/pixmaps`
  (renamed `org.socnetv.SocNetV.png`). A hicolor/scalable icon would be an *added* installed path
  that downstream spec files would reject as unpackaged; the Flatpak manifest places its own SVG
  instead.
- **Out of scope / do not touch:** macOS bundle identifier (`org.socnetv.app`) and the
  `QSettings` organization/domain — changing them would reset users' settings and are unrelated
  to the AppStream ID.

## Audit findings (what hardcodes the old names)

| Location | What | Action |
|---|---|---|
| `CMakeLists.txt` (install block) | desktop, pixmap, appdata install lines | update names |
| `socnetv.pro` (install block) | same three, qmake form | update names |
| `socnetv.spec` | `%files` + `%check` use `%{name}.desktop/.png/.appdata.xml` | update |
| `obs/home:oxy86/socnetv/socnetv.spec` | copy of the above, `_service` pins a release tag | update in step with the tag pin |
| `scripts/travis_make_build_linux.sh` | copies `socnetv.desktop`/`socnetv.png` | update (legacy, but no hardcoded names left behind) |
| `src/images.qrc`, `README.md` | reference `src/images/socnetv.png` | update if that file is renamed |
| `tools/create-ubuntu-package.sh` | `REQUIRED_FILES` lists desktop + appdata | update |
| `tools/update-version.sh` | `bump_appdata()` and editor open use the appdata filename | update |
| `debian/copyright` (Salsa `master` **and** `ubuntu` branch) | `Files: socnetv.appdata.xml` | update in the 3.9 import on both |
| Fedora spec (external, third-party maintained) | builds with **our** `socnetv.pro`; `%files`/`%check` hardcode the three names | ask packager to use globs **before** 3.9 |
| AppImage workflows | no names hardcoded; tooling auto-discovers files | verify with a real build + launch |
| Flathub draft (`scripts/flathub/`) | stale (v3.2, old runtime), own desktop/icon files | rewrite, see Step 8 |

Debian `rules`/`dirs` need no change (CMake install; directories only). Package upload to the
Debian archive stays with the uploader; the Salsa edits are ours to push.

## Steps

Each step ends with its verification. Do not start the next until it passes.

### Step 0 — Fedora packager: switch to globs (blocking, external)
Message sent asking for glob-based `%files`/`%check` that work with both old and new names:
`%{_metainfodir}/*.xml`, `%{_datadir}/applications/*.desktop`, `%{_datadir}/pixmaps/*.png`.
*Verify:* their next build (3.8 or later) still succeeds with the globs against the current
installed names. Do not land Step 2 before this is confirmed.

### Step 1 — Fedora spec dry-run
Build the Fedora spec against a 3.8 tarball and a dev tarball in a container (install a container
runtime on the Linux build host first). Expected: old spec fails on the dev tree (confirms the
risk), globbed spec passes on both. Hand the verified diff to the packager if useful.

### Step 2 — Rename + metadata (on `develop`)
- `git mv` metainfo, desktop file and icon to `org.socnetv.SocNetV.*`; update all rows above.
- Metadata: new `<id>`, `<launchable>`, license `GPL-3.0-or-later`, old ID declared replaced.
- Desktop file: `Icon=org.socnetv.SocNetV`.
- `main.cpp`: `setDesktopFileName`.
*Verify:* AppStream validator (pedantic) on the file and on an installed tree; desktop-file
validator; no leftover references: `grep -rn` for each old name across app repo, `obs/`, `tools/`.

### Step 3 — Install matrix
CMake install and qmake install into temp directories; the two file listings must be identical.
Validate both installed trees.

### Step 4 — Debian
Update `debian/copyright` on Salsa `master` and the `ubuntu` branch for the 3.9 import. Clean-room
build of the dev tree + Debian package linter: no new errors; the metainfo filename warning
must be gone.

### Step 5 — OBS
Update `obs/home:oxy86/socnetv/socnetv.spec` together with the `_service` tag pin bump to 3.9
(never earlier — the pinned tag's tree still has the old names).

### Step 6 — AppImage
CI build (`[ci]` commit), then run the produced AppImage on the Linux host: icon and menu entry
present.

### Step 7 — GUI association (manual, Linux)
On Wayland and X11: launch from the menu entry, confirm the window's icon and dock/taskbar
grouping use the renamed desktop file. (macOS cannot test this.)

### Step 8 — Flathub readiness (#167)
Rewrite the draft manifest: current tag and runtime, app-id `org.socnetv.SocNetV`, SVG installed
to the hicolor scalable icon path by the manifest, desktop/metainfo names matching the app-id,
screenshots reachable, release entries valid. Build locally and run Flathub's manifest/metainfo
linter until clean *before* submitting.

### Step 9 — Release
Only after Steps 0–8 pass. 3.9 release notes mention the rename; downstream packagers are
told in advance (Step 0 already done for Fedora). Fold lasting lessons into
`README__RELEASE_PROCEDURE.md` (e.g. the checklist of hardcoded names) and delete this file.

## Open questions

- Is a container runtime available on the Linux build host? (Needed for Step 1; not installed at
  time of writing.)
- Exact AppStream element for declaring the old ID replaced (check against the spec).
- Are other downstream packagers (Arch, Nix, Homebrew, openSUSE) shipping their own recipes with
  hardcoded names? Unknown; not audited.
- Does the AppImage tooling resolve the renamed pixmap icon without a hicolor copy? (Step 6.)
