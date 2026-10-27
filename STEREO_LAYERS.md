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
New games start with **Per-Layer 3D on** and **Depth Mode = Recommended**, and new installs start with
the video settings above, so you just load a game. The 3D slider on the 3DS still controls how strong the effect is.

## Depth Mode
Open the emulator menu, **Settings** tab, section **3D Layer Depth (this game)**. The mode is saved per game.

- **Auto**: works out the depths while you play. It measures how fast each background
  layer scrolls: the fastest layer is the play field, slower layers are farther away, a layer that
  never scrolls is the HUD (BG3) or a fixed backdrop. On screens where nothing scrolls (title
  screens, menus, name entry) it uses the Recommended depths (below), and moves to its own estimate
  once layers scroll. If you stand still in a game for about 40 seconds it drifts back to the Recommended depths.
- **Recommended** (default): fixed depths. Games in the built-in list (matched by ROM CRC32, see `stereoGameTable` in
  `source/3dsimpl_gpu.cpp`: ActRaiser USA, Yoshi's Island, Super Mario World, Super Mario All-Stars, Super Mario Kart,
  UN Squadron, Uniracers, Bill Laimbeer's Combat Basketball, Earthworm Jim 1 and 2, Aladdin, Goof Troop, Star Fox,
  Star Fox 2 and The Legend of Zelda: A Link to the Past) use depths checked by hand; every other game gets a general preset
  (BG1 -1, BG2 -3, BG3 +3, BG4 -2, Sprites 0). BG1 sits one step behind the sprites so characters pop out of the
  play field. Sprites share one depth, so in some games characters can look slightly like they float; many games in the
  built-in list are at BG1 0 for that reason, and you can set BG1 to 0 in Manual for any other game.
- **Manual**: set a depth (-8..8) for BG1, BG2, BG3, BG4 and Sprites (OBJ). Positive = toward you,
  negative = away. Switching to Manual starts from the depths that were in use. Each slider is labelled with
  what its layer usually holds (BG1 main layer, BG2 back layer, BG3 HUD / text, BG4 extra, Sprites characters),
  but games differ. With **Show Only Selected Layer** checked (default, not saved), the top screen shows just the
  layer whose slider is selected, in a strong colour so it is easy to tell apart (BG1 red, BG2 blue, BG3 yellow,
  BG4 pink, Sprites green; the colour is named on each slider), and moving the slider previews the new depth live.
  Uncheck it to see all layers together: the selected layer stays in its strong colour and the other layers get a
  paler version of theirs, so you can judge the final picture. (Mode 7 backgrounds are not tinted.)
  **Reset Depths to Recommended** puts the five sliders back to the depths Recommended mode uses for this game.

Example Manual depths for ActRaiser action stages: BG1 -1, BG2 -4, BG3 +3, BG4 -2, Sprites 0
(BG1 trees/grass, BG2 far background, BG3 HUD/pause text).

Game configs saved by the first release (v1.61-layers1) keep working: they load as Manual with their own depths.

## Mode 7 Floor Tilt
Settings tab, **3D Layer Depth (this game)**, in every Depth Mode: **Mode 7 Floor Tilt** (0 to 8, default 6, saved per
game). When a game draws a perspective Mode 7 floor (F-Zero, Super Mario Kart, Pilotwings), the floor recedes: its
bottom line stays at BG1's depth and its far end goes back by the tilt, and everything above the floor (sky, horizon)
goes back by the whole tilt, so the sky stays behind the far end of the floor. Sprites and BGs on those lines move with
the floor, so a car stays on the road. Flat Mode 7 (rotating maps, bosses: the same scale on every line) gets no tilt,
and it fades in between, so a game zooming into a floor view does not jump. 0 turns it off.

## Game notes
Depths and notes for the built-in list come from SWChris, who tested them on a New 3DS XL.
- **Super Mario All-Stars**: the Recommended depths (BG1 0, BG2 -4, BG3 -8, BG4 0, Sprites 0) suit Super Mario Bros 1, 2
  and The Lost Levels. For Super Mario Bros 3 use Manual: BG1 0, BG2 -8, BG3 0, BG4 0, Sprites 0. Auto gets confused
  in Super Mario Bros 3 because BG1 and BG3 are layered over each other.
- **UN Squadron**: foreground elements share a layer with the background, so they look inverted.
- **Earthworm Jim 2**: in the Lorenzo's Soil and ISO 9000 levels the game flips BG3's priority, so BG3 becomes the
  foreground there and the depths look wrong. Separate depths for high and low priority tiles on each layer would
  fix this and is on the list of ideas.
- **Star Fox 2**: the default Crop Top/Bottom Scanlines cut off the text boxes, so this game starts with both at 0
  (Settings tab, **Crop & Overscan**). A saved game config keeps whatever you set.
- **The Legend of Zelda: A Link to the Past** uses its layers differently from room to room, so no single setting
  fits every scene. The Recommended entry is Setting A, for open scenes with no layers Link can walk under (BG1 0,
  BG2 0, BG3 +1, BG4 0, Sprites 0). For the other cases switch to Manual and use:
  - Setting B, Link on a bridge or high wall: BG1 -2, BG2 0, BG3 +1, BG4 0, Sprites 0.
  - Setting C, Link can walk under a bridge or overhang, or is on stairs: BG1 -2, BG2 0, BG3 +1, BG4 0, Sprites -1.
  - Setting D, Master Sword glade: BG1 -2, BG2 0, BG3 +3, BG4 0, Sprites -1 (Sprites 0 while Link stands on the white platform).
  - With any of these, sprites on the layer Link is not on look odd, but Link looks right. Switching depths per scene
    would need per-scene profiles, which this fork does not have yet.

## Adding a game to the Recommended list
Open **Emulator** tab, **ROM Info**, note the CRC32. Add a line to `stereoGameTable` in
`source/3dsimpl_gpu.cpp` with your five depths. Pull requests and issue reports with the game name, CRC32 and depths are welcome.

## Known limitations
- Auto is a guess from scroll speed: layers that drift on their own, or games with unusual
  layer use, can look wrong. Switch to Manual for those.
- Mode 7 floor tilt uses one ramp per frame, from the first to the last Mode 7 line. Split-screen Mode 7 (Super Mario
  Kart two player, or a course above a map) gets one ramp over both views, so the upper view sits further back.
- Everything above a Mode 7 floor gets the full tilt, including a HUD drawn there.
- All sprites share one depth. Window/clip masks are not shifted; no per-priority depth within a layer.
- Shifted layers would leave an empty strip at the screen edges, so the layers are built a few tile columns wider
  than the screen and nothing is cropped.

## Building a CIA
The CIA uses its own title ID and name ("Snes9x 3DS Layers") so it installs next to the original Snes9x 3DS
instead of replacing it. Build with `make cia`.

Snes9x keeps its non-commercial license. No ROMs are included; use your own.
