# Per-layer stereoscopic 3D for Snes9x 3DS

Experimental addition to [matbo87/snes9x_3ds](https://github.com/matbo87/snes9x_3ds)
(a fork of bubble2k16's Snes9x for 3DS). Instead of shifting the whole picture, each
SNES layer (BG1-BG4 and sprites) is drawn once per eye at its own depth, so the
game gets real depth on the 3DS top screen.

Developed with Claude (Anthropic) and tested on an Old Nintendo 3DS with ActRaiser,
Donkey Kong Country, Secret of Mana, Super Mario World and Super Metroid, at 60 fps with
Enhanced Resolution "Standard", Scaling "Fullscreen", Scale Filter "Smooth" and Crop & Overscan on.
Not tested on New 3DS.

## Plug and play
New games start with **Per-Layer 3D on** and **Depth Mode = Auto**, and new installs start with
the video settings above, so you just load a game. The 3D slider on the 3DS still controls how strong the effect is.

## Depth Mode
Open the emulator menu, **Settings** tab, section **3D Layer Depth (this game)**. The mode is saved per game.

- **Auto** (default): works out the depths while you play. It measures how fast each background
  layer scrolls: the fastest layer is the play field, slower layers are farther away, a layer that
  never scrolls is the HUD (BG3) or a fixed backdrop. On screens where nothing scrolls (title
  screens, menus, name entry) it fades to flat, and depth fades back in once layers scroll.
  If you stand still in a game for about 40 seconds the depth fades out until you move.
- **Recommended**: fixed depths. Games in the built-in list (by ROM CRC32, currently ActRaiser USA)
  use depths checked by hand; every other game gets a general preset
  (BG1 -1, BG2 -3, BG3 +3, BG4 -2, Sprites 0).
- **Manual**: set a depth (-8..8) for BG1, BG2, BG3, BG4 and Sprites (OBJ). Positive = toward you,
  negative = away. Switching to Manual starts from the depths that were in use.

Example Manual depths for ActRaiser action stages: BG1 -1, BG2 -4, BG3 +3, BG4 -2, Sprites 0
(BG1 trees/grass, BG2 far background, BG3 HUD/pause text).

Game configs saved by the first release (v1.61-layers1) keep working: they load as Manual with their own depths.

## Adding a game to the Recommended list
Open **Emulator** tab, **ROM Info**, note the CRC32. Add a line to `stereoGameTable` in
`source/3dsimpl_gpu.cpp` with your five depths. Pull requests and issue reports with the game name, CRC32 and depths are welcome.

## Known limitations
- Auto is a guess from scroll speed: layers that drift on their own, or games with unusual
  layer use, can look wrong. Switch to Manual for those.
- Mode 7 scenes get no receding depth (BG1's shift at most). Super Mario Kart and F-Zero stay mostly flat.
- All sprites share one depth. Window/clip masks are not shifted; no per-priority depth within a layer.
- Shifted layers can leave a thin empty strip at the screen edges. A fix that crops the
  picture by the widest layer shift was written but never run on hardware, so it is not included.

## Building a CIA
The CIA uses its own title ID and name ("Snes9x 3DS Layers") so it installs next to the original Snes9x 3DS
instead of replacing it. Build with `make cia`.

Snes9x keeps its non-commercial license. No ROMs are included; use your own.
