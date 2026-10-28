
#include <3ds.h>

#include "3dsimpl.h"
#include "3dsgpu.h"
#include "3dssettings.h"
#include "3dsinput.h"
#include "3dsui_notif.h"
#include "3dsutils.h"
#include "memmap.h"
#include "3dssound.h"

static u32 currKeysHeld = 0;
static u32 lastKeysHeld = 0;
static bool ignoreInput = false;
static bool turboModeToggle = false;
// buttons of a combo hotkey (e.g. Select + L) that fired: hidden from the
// game until they are released, so the combo doesn't also press them in-game
static u32 comboKeysHidden = 0;

static u32 hotkeyMask(int hotkey)
{
    return settings3DS.UseGlobalEmuControlKeys ? settings3DS.GlobalButtonHotkeys[hotkey].MappingBitmasks[0]
                                               : settings3DS.ButtonHotkeys[hotkey].MappingBitmasks[0];
}

static bool isCombo(u32 mask)
{
    return (mask & (mask - 1)) != 0;   // more than one button
}

// a combo hotkey that is fully held and uses one of these buttons: a single-
// button hotkey on L then doesn't also fire when SELECT + L is pressed
static bool comboHeldWith(u32 buttons, u32 held)
{
    for (int i = 0; i < HOTKEYS_COUNT; ++i) {
        u32 m = hotkeyMask(i);
        if (isCombo(m) && (m & buttons) && (held & m) == m)
            return true;
    }
    return false;
}

// all of the hotkey's buttons are held and one of them was just pressed, so a
// combo fires on whichever button is pressed last (Select then L, or L then Select)
static bool hotkeyPressed(int hotkey, u32 held, u32 down)
{
    u32 mask = hotkeyMask(hotkey);
    if (mask == 0 || (held & mask) != mask || !(down & mask))
        return false;
    if (isCombo(mask))
        comboKeysHidden |= mask;
    else if (comboHeldWith(mask, held))
        return false;
    return true;
}

static bool input3dsIsFastForwardHoldPressed()
{
    u32 mask = hotkeyMask(HOTKEY_FAST_FORWARD_HOLD);
    if (mask == 0 || (currKeysHeld & mask) != mask)
        return false;
    if (isCombo(mask))
        comboKeysHidden |= mask;
    else if (comboHeldWith(mask, currKeysHeld))
        return false;
    return true;
}

static void input3dsSetTurboMode(bool turboModeActive, bool showNotification)
{
    if (settings3DS.TurboMode == turboModeActive) {
        return;
    }

    settings3DS.TurboMode = turboModeActive;
    if (!showNotification) {
        return;
    }

    notif3dsTrigger(Notif::FastForward, Notif::Type::Info);
}

void input3dsRefreshTurboMode(bool isInGame)
{
    bool fastForwardHeld = isInGame && input3dsIsFastForwardHoldPressed();
    input3dsSetTurboMode(turboModeToggle || fastForwardHeld, isInGame);
}

#ifndef PROFILING_DISABLED
    static void input3dsToggleProfilingMode(bool cycleUp) {
        static const char *profilingModeNames[] = { "Profiling: Off ", "Profiling: Core", "Profiling: All " };
        if (cycleUp)
            GPU3DS.profilingMode = static_cast<SGPU_PROFILING_MODE>((GPU3DS.profilingMode + 1) % (PROFILING_ALL + 1));
        else
            GPU3DS.profilingMode = PROFILING_OFF;
            
        notif3dsTrigger(Notif::Misc, Notif::Type::Info, 1000, profilingModeNames[GPU3DS.profilingMode]);
    }
#endif

//---------------------------------------------------------
// Reads and processes Joy Pad buttons.
//
// This should be called only once every frame only in the
// emulator loop. For all other purposes, you should
// use the standard hidScanInput.
//---------------------------------------------------------
u32 input3dsScanInputForEmulation()
{
    hidScanInput();
    currKeysHeld = hidKeysHeld();
    u32 currentKeysUp = hidKeysUp();

    if (ignoreInput) {
        if (currentKeysUp != 0 || currKeysHeld == 0) {
            ignoreInput = false;
        } else {
            // no keys are pressed
            currKeysHeld = 0;
        }
    }
    
    u32 keysDown = (~lastKeysHeld) & currKeysHeld;
    bool isInGame = GPU3DS.emulatorState == EMUSTATE_EMULATE;
    comboKeysHidden &= currKeysHeld;   // released buttons go back to the game

    if (keysDown & KEY_TOUCH || hotkeyPressed(HOTKEY_OPEN_MENU, currKeysHeld, keysDown))
    {
        if (isInGame)
        {
            if (settings3DS.ForceSRAMWriteOnPause || CPU.SRAMModified)
            {
                S9xAutoSaveSRAM();
            }
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
            snd3dsDrainMixing();
            notif3dsHide();
        }
    }
    
    if (isInGame) {
        #ifndef PROFILING_DISABLED
        if ((currKeysHeld & KEY_SELECT) && (currKeysHeld & KEY_L) && (currKeysHeld & KEY_RIGHT) && (keysDown & KEY_RIGHT)) {
            input3dsToggleProfilingMode(true);
        }
        if ((currKeysHeld & KEY_SELECT) && (currKeysHeld & KEY_L) && (currKeysHeld & KEY_LEFT) && (keysDown & KEY_LEFT)) { 
            input3dsToggleProfilingMode(false);
        }
        #endif

        if (hotkeyPressed(HOTKEY_SWAP_CONTROLLERS, currKeysHeld, keysDown))
            impl3dsSwapJoypads();
            
        if (hotkeyPressed(HOTKEY_QUICK_SAVE, currKeysHeld, keysDown))
            impl3dsQuickSaveLoad(true);

        if (hotkeyPressed(HOTKEY_QUICK_LOAD, currKeysHeld, keysDown))
            impl3dsQuickSaveLoad(false);
        
        if (hotkeyPressed(HOTKEY_SAVE_SLOT_NEXT, currKeysHeld, keysDown))
            impl3dsSelectSaveSlot(1);

        if (hotkeyPressed(HOTKEY_SAVE_SLOT_PREV, currKeysHeld, keysDown))
            impl3dsSelectSaveSlot(-1);
            
        if (hotkeyPressed(HOTKEY_SCREENSHOT, currKeysHeld, keysDown)) {
            impl3dsPrepareScreenshot();
        }

        if (hotkeyPressed(HOTKEY_SWAP_DEPTH_PROFILE, currKeysHeld, keysDown)) {
            settings3dsSwapDepthProfile();
            GPU3DS.gameScreenBufferDesync = true;
            notif3dsTrigger(Notif::Misc, Notif::Type::Info, 1000,
                settings3DS.StereoProfile ? "Depth profile B" : "Depth profile A");
        }

        if (hotkeyPressed(HOTKEY_TOGGLE_3D_LAYERS, currKeysHeld, keysDown)) {
            settings3DS.Stereo3DLayers = !settings3DS.Stereo3DLayers;
            GPU3DS.gameScreenBufferDesync = true;
            notif3dsTrigger(Notif::Misc, Notif::Type::Info, 1000,
                settings3DS.Stereo3DLayers ? "Per-layer 3D on" : "Per-layer 3D off");
        }

        bool fastForwardTogglePressed = hotkeyPressed(HOTKEY_FAST_FORWARD_TOGGLE, currKeysHeld, keysDown);

        if (fastForwardTogglePressed) {
            turboModeToggle = !turboModeToggle;
        }
    }
    input3dsRefreshTurboMode(isInGame);


    lastKeysHeld = currKeysHeld;
    return keysDown;

}

void input3dsWaitForRelease()
{
    ignoreInput = true;
}

//---------------------------------------------------------
// Get the bitmap of keys currently held on by the user
//---------------------------------------------------------
u32 input3dsGetCurrentKeysHeld()
{
    return currKeysHeld & ~comboKeysHidden;
}
