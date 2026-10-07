# AppStream Identity & Packaging Names (WS19) — transient

## Goal

Issue #309: use one standard application ID, `org.socnetv.SocNetV`, for the AppStream metadata
file, desktop file, icon and window/launcher association, correct the metadata license, and leave
the project Flathub-ready (#167) — **without breaking** any existing distribution channel.

Transient workstream: scoped to the 3.9 cycle. Fold the lasting parts (naming convention,
verification matrix) into the release procedure and delete this file once shipped.

## Status

Planned. Nothing implemented. Audit complete. Step 1 done; Steps 2-8 proceed on `develop`; the
3.9 *release* (Step 9) is gated on downstream packagers (Step 0), not the development work.

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

### Step 0 — Fedora packager: switch to globs (release gate, external)
Message sent asking for glob-based `%files`/`%check` that work with both old and new names:
`%{_metainfodir}/*.xml`, `%{_datadir}/applications/*.desktop`, `%{_datadir}/pixmaps/*.png`.
*Verify:* their next build (3.8 or later) still succeeds with the globs against the current
installed names. Gates the 3.9 *tag* (Step 9) only: Fedora builds from release tags, so work on `develop` is
unaffected. If not confirmed in time, options: send them the tested diff as a PR, or defer the
rename to the next release. Existing Fedora installs are unaffected either way; only a Fedora
update to the renamed release would fail to build.

### Step 1 — Fedora spec dry-run
Status: ✅ done. Built the Fedora spec in a rawhide container against a 3.8 tarball and a copy
with the three files renamed (simulated; the app repo was not touched):

| Spec | 3.8 as-is | renamed tree |
|---|---|---|
| current (names hardcoded) | pass | **fail** — `%check`: desktop file `socnetv.desktop` does not exist |
| globbed `%files` + `%check` | pass | pass |

The globbed spec is safe to adopt before 3.9; without it 3.9 would not build on Fedora. `%check`
fails first, so the `%files` failure itself was not exercised separately.

### Step 2 — Rename + metadata (on `develop`)

Status: done (commits on `develop`; Linux CI configure/build fixed for the icon path).

Findings that shape it:
- Old names live in `CMakeLists.txt` (3 install lines), `socnetv.pro` (3), `socnetv.appdata.xml`
  (`<launchable>`), `scripts/travis_make_build_linux.sh`, `src/images.qrc`, `README.md`, and
  `socnetv.spec` (via `%{name}.*`).
- The qrc entry `images/socnetv.png` is the only reference to the icon in `src/`; no code loads it.
- The installed icon is 64x64 px. Flathub wants 128 px or SVG; the manifest installs its own SVG
  (Step 8), so the PNG stays as is. Replacing its pixels later is not a path change.
- The CMake install block is Linux-only (`UNIX AND NOT APPLE`): a macOS build cannot test it.
- `tools/update-version.sh` `bump_appdata()` skips silently when its hardcoded filename is
  missing, so after the rename the next release bump would silently drop the `<release>` entry.

Commits (small, separable; none closes #309 — that happens at Step 9):
1. Rename + build wiring, atomic: `git mv` the desktop file, metainfo and icon to
   `org.socnetv.SocNetV.*`; update `CMakeLists.txt`, `socnetv.pro`, `images.qrc`, `README.md`,
   `scripts/travis_make_build_linux.sh`, `socnetv.spec` (`%files` + `%check`).
2. Metadata contents: new `<id>`, `<launchable>`, license `GPL-3.0-or-later`, old ID declared
   replaced; desktop file `Icon=org.socnetv.SocNetV`.
3. `main.cpp`: `app.setDesktopFileName("org.socnetv.SocNetV")` next to `setApplicationName`.
4. Tools repo (separate): `update-version.sh` (filename in `bump_appdata` and the editor call) and
   `create-ubuntu-package.sh` (`REQUIRED_FILES`) — together with commit 1.
5. The last commit of the series carries `[ci]` to trigger the AppImage build for Step 6.

Assumptions to verify, not assume:
- The replaced-ID element is accepted by the AppStream validator (pedantic) — check on the Linux
  host before commit 2; drop it if not (the old ID is then simply orphaned).
- X11 may need `StartupWMClass=` in the desktop file; measure the real `WM_CLASS` first (Step 7).
- Wayland association via `setDesktopFileName` needs a real check (Step 7).

Verification: repo-wide grep shows no old names left (tools repo greps separately); full macOS
build + launch (qrc change); `run_golden_compares.sh` (commit 3 is a code change); then Step 3.

### Step 3 — Install matrix
Status: done on a Linux host (Qt 6.8.3, source copy from `git archive`).
- CMake and qmake installs are identical except `usr/share/doc/socnetv/*` (CMake installs docs;
  qmake does not — expected). All three renamed files are present at the same paths in both; no old
  names remain.
- AppStream validator on both installed trees: the filename/ID mismatch warning is gone. Only a
  pedantic-level note remains for the CamelCase ID (decision: keep, Flathub convention). Desktop
  file validates for both.
- The replaced-ID element validates (pedantic) and was added.
- Optional polish, not part of this workstream: the desktop file `Categories=` has more than one
  main category (validator hint, pre-existing).

### Step 4 — Debian
Do at release time, with the 3.9 import. `debian/copyright` names the metadata file in a
`Files:` stanza declaring it CC0-1.0, on both Salsa `master` and the `ubuntu` branch; after the
rename the stanza must follow it, or the file silently falls under the GPL-3 wildcard. The exact
procedure (ordering around the Salsa script's resume mode, the `ubuntu` branch timing, a filename
check against the release tarball) is written down in the release procedure doc, "Step D".
*Verify:* clean-room build of the 3.9 tree + Debian package linter: no new errors; the metainfo
filename warning is gone.

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
Only after Steps 0–8 pass and Step 0 is confirmed. 3.9 release notes mention the rename; downstream packagers are
told in advance (Step 0 already done for Fedora). Fold lasting lessons into
`README__RELEASE_PROCEDURE.md` (e.g. the checklist of hardcoded names) and delete this file.

## Open questions

- Is a container runtime available on the Linux build host? (Needed for Step 1; not installed at
  time of writing.)
- Exact AppStream element for declaring the old ID replaced (check against the spec).
- Are other downstream packagers (Arch, Nix, Homebrew, openSUSE) shipping their own recipes with
  hardcoded names? Unknown; not audited.
- Does the AppImage tooling resolve the renamed pixmap icon without a hicolor copy? (Step 6.)
