#ifndef _3DSGUIDE_H_
#define _3DSGUIDE_H_

// Guide viewer: a game's plain-text guide on the bottom screen while it runs
// (the same viewer as gpSP 3D's). <game>.txt next to the game, or in
// sdmc:/3ds/snes9x_3ds/guides/.

void guide3dsOpenForRom(const char *romPath);   // after a game is loaded
void guide3dsClose();                           // remembers the place
bool guide3dsAvailable();
const char *guide3dsError();                    // why it couldn't be loaded, or NULL
bool guide3dsUsable();                          // available and the bottom screen is free
bool guide3dsShown();
void guide3dsSetShown(bool shown);
void guide3dsSavePos();

// emulation loop: show the guide or the normal bottom screen after a change
bool guide3dsScreenChanged();                   // clears the flag
void guide3dsBeginShowing();                    // single buffering + draw
// touch while shown; returns true when the Menu button was tapped
bool guide3dsHandleTouch(u32 keysDown, u32 keysHeld, u32 keysUp);

#endif
