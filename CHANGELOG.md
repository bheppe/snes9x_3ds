# Changelog
Notable changes to this project will be documented in this file.

## v1.61-layers8 (per-layer 3D fork, based on v1.61)

### Changes
* **Hotkey combos.** Each hotkey can now be a two-button combo: SELECT with L / R / A / B / X / Y / the D-pad, START + L / R, L + R, and ZL + ZR on New 3DS. A combo fires when its last button is pressed, in either order, and its buttons are hidden from the game until released. Asked for by cabjoy.
* **Two depth profiles per game.** **Settings → 3D Layer Depth → Depth Profile**: A or B, each with its own Depth Mode and depths (e.g. one for the overworld, one for battles in games that swap their layers, like EarthBound). The new **Swap Depth Profile** hotkey switches in game. A **Per-Layer 3D On/Off** hotkey compares with the flat 3D picture while you tune depths. Game config v1.9, global config v1.8; older configs start with B as a copy of their one profile. Asked for by Finn.
* **No Stretch is true 1:1.** Without Overscan, one SNES pixel is now exactly one screen pixel (a column and a row used to be dropped). Reported by Finn.
* **Guide viewer on the bottom screen**, like gpSP 3D's: a plain-text guide next to the game (`Game.txt`) or in `sdmc:/3ds/snes9x_3ds/guides/`, shown with **Show Guide on Bottom Screen** in the in-game menu or the new **Guide On/Off** hotkey. Drag or flick to scroll; it remembers your place per game. Asked for by cabjoy.
* **Mario is Missing!** in the Recommended depth list (CRC32 and depths from SWChris).
* **New app icon** (an "s9x" monogram, easy to tell apart from gpSP 3D), a **3D HOME Menu banner** (the icon on a turning tile, the name under it) and a soft chime instead of the startup sound. The app also allows 3D in its SMDH, which smdhtool left out.
* bheppe is listed as an author on the HOME Menu and in FBI.

## v1.61-layers7 (per-layer 3D fork, based on v1.61)

### Changes
* **Mode 7 floor tilt.** Perspective floors (F-Zero, Super Mario Kart, Pilotwings) now recede into the screen in 3D instead of sitting flat, and the sky stays behind them. New per-game setting under **Settings → 3D Layer Depth (this game)**: **Mode 7 Floor Tilt**, 0 to 8 (default 6, 0 = flat), saved as `StereoMode7Tilt` in the game config (config v1.8). With 3D off or the slider at 0 nothing changes. By hspman69 (#2).
* **The picture fills the whole screen in 3D.** Per-layer 3D used to crop the sides of the picture; extra tile columns are now drawn at the edges instead. By hspman69 (#2).

## v1.61-layers6 (per-layer 3D fork, based on v1.61)

### Changes
* **HOME Menu icons for SNES games.** Snes9x 3D can now be started straight into a game from its own HOME Menu icon. Make the icons on the 3DS with **Icon Maker** (attached to this release; source in [gpsp-3d](https://github.com/bheppe/gpsp-3d/tree/main/iconmaker)): pick a game, press A, and its icon and banner go on the HOME Menu. An icon writes the game's path to `sdmc:/3ds/snes9x_3ds/launch.txt` and starts the CIA; at startup Snes9x 3D reads and deletes that file and loads the game as if it had been picked in the file list (loading screen, auto save state, RetroAchievements). Started normally, it shows the menu as before.

## v1.61-layers5 (per-layer 3D fork, based on v1.61)

### Changes
* 12 more games in the Recommended list, with CRC32s and depths from SWChris: Super Mario All-Stars, Super Mario Kart, UN Squadron, Uniracers, Bill Laimbeer's Combat Basketball, Earthworm Jim, Earthworm Jim 2, Aladdin, Goof Troop, Star Fox, Star Fox 2 and The Legend of Zelda: A Link to the Past (its open-scene setting; the depths for bridges, stairs and the Master Sword glade are in the new Game notes section of [STEREO_LAYERS.md](STEREO_LAYERS.md)).
* Star Fox 2 starts with Crop Top Scanlines and Crop Bottom Scanlines at 0, so the text boxes are no longer cut off (a saved game config keeps its own values). Suggested by SWChris.

## v1.61-layers4 (per-layer 3D fork, based on v1.61)

### Changes
* Yoshi's Island and Super Mario World are now in the Recommended list (BG1 0, BG2 -4, BG3 +3, BG4 -2, Sprites 0): sprites share one depth, so BG1, the layer the characters stand on, sits at the same depth and they no longer look like they float, and BG2 -4 fixes the flat mountain layer in Yoshi's Island Level 1. CRC32s and depths from SWChris. All other games keep the general preset (BG1 -1, BG2 -3, BG3 +3, BG4 -2, Sprites 0).
* With **Show Only Selected Layer** unchecked, the top screen now shows all layers together with the selected layer highlighted in its strong colour and the other layers in a paler version of theirs, so you can judge the final picture. Checked still shows only the selected layer. Suggested by Chompsky__Honk.

## v1.61-layers3 (per-layer 3D fork, based on v1.61)

See [STEREO_LAYERS.md](STEREO_LAYERS.md) for how it works.

### Features
* Per-layer stereoscopic 3D: BG1-BG4 and sprites are drawn once per eye at their own depth (Settings tab → 3D Layer Depth (this game)).
* Depth Mode per game: Recommended (default for new games), Auto (estimated from layer scroll speeds; uses the Recommended depths where nothing scrolls, so 3D stays on) or Manual (five depth sliders).
* Manual depth sliders are labelled with what each layer usually holds, and **Show Only Selected Layer** shows just the selected layer on the top screen in its own colour (BG1 red, BG2 blue, BG3 yellow, BG4 pink, sprites green). Moving a slider previews the new depth live.
* Added **Reset Depths to Recommended** (Manual mode).
* Recommended list entry for ActRaiser (USA); other games get a general preset.
* The picture is cropped slightly at the left and right edges by the widest layer shift, to hide the empty strips shifted layers leave.
* The CIA installs as "Snes9x 3DS Layers" with its own title ID, next to the original Snes9x 3DS.

## v1.61

### Features
* Added 3D depth strength setting and refined splash/background depth effects ([#65](https://github.com/matbo87/snes9x_3ds/issues/65)) ([c08c7f5](https://github.com/matbo87/snes9x_3ds/commit/c08c7f5c), [ef677f9](https://github.com/matbo87/snes9x_3ds/commit/ef677f9f))
* Added save-state screenshot previews ([9520798](https://github.com/matbo87/snes9x_3ds/commit/9520798f), [5a959fd](https://github.com/matbo87/snes9x_3ds/commit/5a959fd7))
* Added ROM Info dialog ([52fde3e](https://github.com/matbo87/snes9x_3ds/commit/52fde3e9))
* Added per-game crop/overscan control ([#55](https://github.com/matbo87/snes9x_3ds/issues/55)) ([c24f52c](https://github.com/matbo87/snes9x_3ds/commit/c24f52cb), [9cd70ac](https://github.com/matbo87/snes9x_3ds/commit/9cd70acb), [dd3ece5](https://github.com/matbo87/snes9x_3ds/commit/dd3ece5c))
* Added scanlines ([e739def](https://github.com/matbo87/snes9x_3ds/commit/e739def8))
* Added Mode 7 bilinear smoothing ([#68](https://github.com/matbo87/snes9x_3ds/pull/68))
* Added Frame Sync setting with VBlank/Sleep pacing options ([1692be4](https://github.com/matbo87/snes9x_3ds/commit/1692be44))
* Added audio buffer size setting ([6b5e022](https://github.com/matbo87/snes9x_3ds/commit/6b5e022b))

### Rendering & Compatibility
* Fixed HDMA/in-frame palette compatibility for games with mid-frame palette changes ([#73](https://github.com/matbo87/snes9x_3ds/pull/73))
* Added mosaic rendering support ([#70](https://github.com/matbo87/snes9x_3ds/pull/70))
* Fixed Mode 7 stale tile/texture issues ([e754a64](https://github.com/matbo87/snes9x_3ds/commit/e754a649), [b9080be](https://github.com/matbo87/snes9x_3ds/commit/b9080be1))
* Fixed stale core data after switching ROMs ([21ae864](https://github.com/matbo87/snes9x_3ds/commit/21ae8640), [d9e1932](https://github.com/matbo87/snes9x_3ds/commit/d9e19329))
* Fixed several game-specific rendering/timing issues ([93f6fd6](https://github.com/matbo87/snes9x_3ds/commit/93f6fd60), [56d46ee](https://github.com/matbo87/snes9x_3ds/commit/56d46ee4), [84fd3b7](https://github.com/matbo87/snes9x_3ds/commit/84fd3b76), [0045f54](https://github.com/matbo87/snes9x_3ds/commit/0045f54d), [5c49b44](https://github.com/matbo87/snes9x_3ds/commit/5c49b442))
* Optimized Mode 7 tile 0 and static palette updates ([#67](https://github.com/matbo87/snes9x_3ds/pull/67), [ac40c53](https://github.com/matbo87/snes9x_3ds/commit/ac40c538))

### Other Improvements
* Migrated audio output from CSND to NDSP and improved audio scheduling/stability ([#58](https://github.com/matbo87/snes9x_3ds/pull/58), [7ff81cc](https://github.com/matbo87/snes9x_3ds/commit/7ff81cc1), [f33eccf](https://github.com/matbo87/snes9x_3ds/commit/f33eccf4), [a300cf0](https://github.com/matbo87/snes9x_3ds/commit/a300cf05))
* Reduced `cpuexec.o` I-cache pressure and improved layout stability on Old 3DS ([#66](https://github.com/matbo87/snes9x_3ds/pull/66))
* Added detection and warning for savestates with a broken-audio signature ([10199ee](https://github.com/matbo87/snes9x_3ds/commit/10199ee8))
* Improved Citra compatibility for Mode 7 rendering and emulator detection ([861b714](https://github.com/matbo87/snes9x_3ds/commit/861b7147), [a17bea0](https://github.com/matbo87/snes9x_3ds/commit/a17bea09))


## v1.60.2

### Bug Fixes
* Fixed in-game freeze after toggling "Disable 3D" in menu ([#54](https://github.com/matbo87/snes9x_3ds/issues/54)) ([5251996](https://github.com/matbo87/snes9x_3ds/commit/52519966))
* Fixed SNES core regressions introduced by earlier cleanup commits ([14af419](https://github.com/matbo87/snes9x_3ds/commit/14af419), [fb200ab](https://github.com/matbo87/snes9x_3ds/commit/fb200abb))

### Other Improvements
* Reintroduced fast-forward hold hotkey and preserved legacy config compatibility ([#23](https://github.com/matbo87/snes9x_3ds/issues/23)) ([ce600fc](https://github.com/matbo87/snes9x_3ds/commit/ce600fc1))
* Minor UI adjustments ([e097bb6](https://github.com/matbo87/snes9x_3ds/commit/e097bb6d), [5b6188a](https://github.com/matbo87/snes9x_3ds/commit/5b6188a))


## v1.60.1

### Bug Fixes
* Fixed VRAM read control flow regression ([#46](https://github.com/matbo87/snes9x_3ds/issues/46)) ([c32c5ab](https://github.com/matbo87/snes9x_3ds/commit/c32c5ab))
* Fixed WindowLR overlap tagging when trimming black scanlines ([#46](https://github.com/matbo87/snes9x_3ds/issues/46)) ([d536983](https://github.com/matbo87/snes9x_3ds/commit/d536983))

### Reintroduced Features
* Reintroduced optional screen smoothing for stretched modes ([#51](https://github.com/matbo87/snes9x_3ds/issues/51)) ([432d202](https://github.com/matbo87/snes9x_3ds/commit/432d202))
* Reintroduced per-game framerate override (Auto or Force 60 FPS) ([#50](https://github.com/matbo87/snes9x_3ds/issues/50)) ([b4f45e8](https://github.com/matbo87/snes9x_3ds/commit/b4f45e8))

### Maintenance
* Document bundled makerom sources for provenance ([#47](https://github.com/matbo87/snes9x_3ds/issues/47)) ([4cae630](https://github.com/matbo87/snes9x_3ds/commit/4cae630))
* CI/tooling updates for GitHub Actions compatibility ([475042a](https://github.com/matbo87/snes9x_3ds/commit/475042a), [7e1a91a](https://github.com/matbo87/snes9x_3ds/commit/7e1a91a))


## v1.60

### Major Changes
* **Rendering backend migration**: move from legacy GPU code to citro3d
* **Draw-call batching overhaul**: fewer draw calls via batched rendering and XOR-based packed render-state diffing
* **GPU decoupling**: separate `gfxhw` state preparation from the GPU submission path for a cleaner rendering pipeline

### Performance
* **Rendering throughput**:
  * layer/section collection and merged backdrop/color-math passes to reduce redundant draws
  * uniform upload and render-state update optimizations (including patched citro3d max-dirty behavior)
* **I/O and memory**:
  * faster save/config writes and improved file I/O architecture
  * reduced heap fragmentation pressure
  * menu/file navigation streamlined, with snappier behavior on old 2DS/3DS models
  * improved ROM list caching
* **Asset handling**:
  * background assets on 16-bit texture formats (RGB565) to reduce memory bandwidth/footprint
  * replace `stb_image` with a `libpng`-based path that uses a shared file scratch buffer (`g_fileBuffer`) and an aligned shared stream buffer (`g_streamBuffer`) to reduce heap churn/fragmentation

### Features
* **Thumbnail system**:
  * replace fragile thumbnail background-thread loading with on-demand reads from one cache file per thumbnail type
  * removes shared-state race issues and keeps thumbnail loading fast and stable for large ROM folders
* **SNES-accurate refresh-rate matching**:
  * when gameplay starts/resumes, 3DS LCD timing is set to the game's native SNES rate (NTSC ~60.1 Hz / PAL 50 Hz)
* **On-Screen Display**:
  * bezel overlay with auto-fit support
  * FPS overlay option
  * GPU-accelerated notifications
* **Stereoscopic 3D additions**:
  * basic 3D support for splash screen, in-game scene background, and pause overlay

### Stability & Code Quality
* **Code-quality cleanup**: broad typing, const/sign correctness, return-path, warning cleanup across both 3DS frontend and SNES core
* **Build warning policy upgrade**: remove old global warning suppression (`-w`) and move to enabled warnings enforcing `-Werror` by default

### Breaking Changes
* **Config migration**: `settings.cfg` may not migrate cleanly in all cases; defaults can be applied
* **Thumbnail assets**: legacy per-image thumbnail folders are obsolete; thumbnails now load from `*.cache` files (`boxart.cache`, `gameplay.cache`, `title.cache`)
* **Background asset paths**:
  * `snes9x_3ds/borders` -> `snes9x_3ds/backgrounds/game_screen`
  * `snes9x_3ds/covers` -> `snes9x_3ds/backgrounds/second_screen`


## v1.52

### Bug Fixes
* **Thread safety**: prevent cache thread from accessing `romFileNames` while it is being modified by the main thread ([#32](https://github.com/matbo87/snes9x_3ds/issues/32)) ([ea806c5](https://github.com/matbo87/snes9x_3ds/commit/ea806c5d89d186f1d9018a8d1850190d422ad4ca))
* **Shutdown stability**: ensure all global/static stores are cleared properly at exit to avoid late destruction‑order crashes ([#32](https://github.com/matbo87/snes9x_3ds/issues/32)) ([ea806c5](https://github.com/matbo87/snes9x_3ds/commit/ea806c5d89d186f1d9018a8d1850190d422ad4ca))
* **ROM mapping**: Fix incorrect memory bank mapping for Mega Man X (and probably other ROMs) ([#26](https://github.com/matbo87/snes9x_3ds/issues/26)) ([05c6663](https://github.com/matbo87/snes9x_3ds/commit/05c6663ae2d944c4c232838ac4f5bf0d8c6c98aa))

### Features
* **Screen stretch**: add "8:7 Fit" scaling option ([#28](https://github.com/matbo87/snes9x_3ds/pull/28)) ([526d62f](https://github.com/matbo87/snes9x_3ds/commit/526d62f9250421ed867c43a675c226c63b718f19))
* **Screen filter**: add "linear filtering" option ([#28](https://github.com/matbo87/snes9x_3ds/pull/28)) ([9744318](https://github.com/matbo87/snes9x_3ds/commit/9744318cc747adeaf0c7decff94c8126584fa8b4))

### Code Refactoring
* **Performance**: revert commit 8d50f5 due to negative performance impact ([d50de94](https://github.com/matbo87/snes9x_3ds/commit/d50de943ff0415eb70f8cec3dfc7e50fe1490886))
* **Shader**: remove unused shaders + adjust makefile ([3aa0377](https://github.com/matbo87/snes9x_3ds/commit/3aa03772cecf17264e4cfee00545360286c15a42))


## v1.51.1

### Bug Fixes
* **Old 3DS, Old 2DS**: fix crash on O3DS/O2DS when user opens menu after game has loaded ([#11](https://github.com/matbo87/snes9x_3ds/issues/11)) ([71ed471](https://github.com/matbo87/snes9x_3ds/commit/71ed471f3f9bfea74f42105ddbbbf3b9e9a94c07))


## v1.51

### Features
* **Theme option:** add Dark mode and RetroArch theme ([#4](https://github.com/matbo87/snes9x_3ds/issues/2)) ([d343ca6](https://github.com/matbo87/snes9x_3ds/commit/d343ca60fb0e380fa9b4239c7ebf346e0ff86e6c))
* **File menu:** adjust navigation pattern + provide more options in file menu tab ([#4](https://github.com/matbo87/snes9x_3ds/issues/2)) ([d343ca6](https://github.com/matbo87/snes9x_3ds/commit/d343ca60fb0e380fa9b4239c7ebf346e0ff86e6c), [ea2cd3f](https://github.com/matbo87/snes9x_3ds/commit/ea2cd3fa970f81a4384ebf0c7b014b429d4d7d34))
  * Going up a directory by pressing B
  * Option to set a default starting folder
  * Delete game option
  * Random game option
* **Pause screen:** show a decent pause screen when menu is open during gameplay ([4c9f3ec](https://github.com/matbo87/snes9x_3ds/commit/4c9f3ecb333eaf23da85e9199bdbbfa3511312dd))

### Bug Fixes
* **O2DS**: fix crash on O2DS (and probably O3DS as well) when saving SRAM ([#2](https://github.com/matbo87/snes9x_3ds/issues/2)) ([02788b1](https://github.com/matbo87/snes9x_3ds/commit/02788b17d038e30e612dcbf0719ec45a8fc54a43))

### Code Refactoring
* **Menu**: reduce redundant code + preserve selected item index per tab ([493c1a2](https://github.com/matbo87/snes9x_3ds/commit/493c1a22b3975c7cb39a55dbd38140e5e3cd2a14), [4d6378a](https://github.com/matbo87/snes9x_3ds/commit/4d6378a507cb77571e4444abb6fbd0df3ff5f555))
* **Dialogs**: remove unnecessary animations for a snappier appearance ([2bb82c6](https://github.com/matbo87/snes9x_3ds/commit/2bb82c69512a2ef894ee5bb049be13ba567b6e89))
* **Second screen content**: clean up + prevent flickering when info dialog appears/disappears ([c1899df](https://github.com/matbo87/snes9x_3ds/commit/c1899df01828b9653c3c635695e61d1ce4fbeaee))


## v1.50

### Features
* **Game preview option:** boxart, title or gameplay
* **Improved cheat menu:** now with available/activated indicator
* **Updated Banner:** based on SNES VC banner
* **Menu clean up:**
  * Reset config(s) option
  * Autosave is now game-specific
  * Show saving dialog instead of _freezed_ menu

### Bug Fixes
* **Cheats**: Fix cheats not loaded/saved properly
* **Default button mappings**: Fix missing default controls 
* **Home menu button:** make emulator quit properly when user exits via home menu button
* **Pixel perfect mode:** Fix blurry image (mentioned [here](https://github.com/asdolo/snes9x_3ds_forwarder/pull/1))
* **Long game lists**: Fix app crash on exit

### Code Refactoring
* **Makefile & app.rsf:** use TricksterGuy's [3ds-template](https://github.com/TricksterGuy/3ds-template), update compiler options
* **Image loading/rendering:** use stb_image instead of lodepng for faster image decoding, unify image rendering logic
* **Second screen content**: improve performance

### Breaking changes
* **Folder structure:** All game related files are now in "3ds/snes9x_3ds", similar to RetroArch folder structure


---
## Older releases

### v1.45
- Buffered file writer for faster config saves (thanks to [willjow](https://github.com/willjow/snes9x_3ds))

### v1.42
- Fixed screen tearing
- Added option to disable 3D Slider (thanks to ramzinouri)

### v1.41
- Fixed hotkey for making screenshot 
- Fixed quick save/load (no data abort exceptions anymore)
- Fixed Errors if cover image is missing
- Updated assets (icon, banner, border, cover)

### v1.40

- Added Swap Game Screen option
- Added switch controller option like in official Virtual Console (SF2 "Training Mode", Konami cheat, ...)
- Custom second screen image and border for every game (thanks to ramzinouri and Asdolo)
- Game Info option for second screen
- Provide more Hotkeys (Quick Save/Load, Swap Controllers)
- Disable Analog to Digital Type option which allows you to use circle pad for hotkeys as well
- All game related files like cheats or save states are now in a single folder (folder name = rom name)
- Screenshots are now in PNG format (thanks to ramzinouri)
- Removed BlargSNES DSP Core, updated dsp-1, added dsp-2 -3 and -4 (thanks to ramzinouri)

### [bubble2k Change Log](https://github.com/bubble2k16/snes9x_3ds#change-history)
