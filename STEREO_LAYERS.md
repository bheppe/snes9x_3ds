# Per-layer stereoscopic 3D for Snes9x 3DS

Experimental addition to [matbo87/snes9x_3ds](https://github.com/matbo87/snes9x_3ds)
(a fork of bubble2k16's Snes9x for 3DS). Instead of shifting the whole picture, each
SNES layer (BG1-BG4 and sprites) is drawn once per eye at its own depth, so the
game gets real depth on the 3DS top screen.

Developed with Claude (Anthropic) and tested on an Old Nintendo 3DS with ActRaiser (USA):
60 fps with Enhanced Resolution "Standard", Scaling "Fullscreen", Scale Filter "Smooth",
Crop & Overscan on. Not tested with other video settings or on New 3DS.

## Use
1. In the emulator menu open the **Settings** tab, section **3D Layer Depth (this game)**.
2. Tick **Per-Layer 3D**.
3. Set a depth (-8..8) for BG1, BG2, BG3, BG4 and Sprites (OBJ). Positive = toward you,
   negative = away. Depths scale with the 3DS 3D slider and are saved per game.

Example for ActRaiser action stages: BG1 -1, BG2 -4, BG3 +3, BG4 -2, Sprites 0
(BG1 trees/grass, BG2 far background, BG3 HUD/pause text).

## Known limitations
- Mode 7 scenes get no receding depth (BG1's shift at most).
- Window/clip masks are not shifted; no per-priority depth within a layer.
- Shifted layers can leave a thin empty strip at the screen edges. A fix that crops the
  picture by the widest layer shift was written but never run on hardware, so it is not included.

Snes9x keeps its non-commercial license. No ROMs are included; use your own.
