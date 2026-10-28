#include <3ds.h>
#include <string.h>
#include <stdio.h>

#include "3dsguide.h"
#include "3dsimpl.h"
#include "3dsgpu.h"
#include "3dssettings.h"
#include "3dsguide_view.h"
#include "3dsguide_ui.h"

static bool screenChanged = false;

void guide3dsOpenForRom(const char *romPath)
{
    const char *slash = strrchr(romPath, '/');
    char name[256];
    snprintf(name, sizeof(name), "%s", slash ? slash + 1 : romPath);
    char *dot = strrchr(name, '.');
    if (dot)
        *dot = '\0';
    guide_open(romPath, name, "sdmc:/3ds/snes9x_3ds");
    screenChanged = true;
}

void guide3dsClose()              { guide_close(); }
bool guide3dsAvailable()          { return guide_available(); }
const char *guide3dsError()       { return guide_error(); }
void guide3dsSavePos()            { guide_save_pos(); }

// the guide is drawn for a 320x240 bottom screen
bool guide3dsUsable()
{
    return guide_available() && settings3DS.SecondScreen == GFX_BOTTOM;
}

bool guide3dsShown()
{
    return guide3dsUsable() && guide_shown;
}

void guide3dsSetShown(bool shown)
{
    if (guide_shown == shown)
        return;
    guide_shown = shown;
    guide_save_pos();
    screenChanged = true;
}

bool guide3dsScreenChanged()
{
    bool changed = screenChanged;
    screenChanged = false;
    return changed;
}

// copy the 320x240 RGB565 canvas to the bottom framebuffer (240x320, column-major, y flipped)
static void present()
{
    u16 w = 0, h = 0;
    u8 *fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &w, &h);
    if (!fb || w != UI_H || h != UI_W)
        return;

    GSPGPU_FramebufferFormat fmt = gfxGetScreenFormat(GFX_BOTTOM);
    int bpp = fmt == GSP_BGR8_OES ? 3 : (fmt == GSP_RGBA8_OES ? 4 : 2);

    for (int x = 0; x < UI_W; x++) {
        u8 *col = fb + x * UI_H * bpp;
        for (int y = 0; y < UI_H; y++) {
            u16 c = ui_canvas[y * UI_W + x];
            u8 *p = col + (UI_H - 1 - y) * bpp;
            if (bpp == 2) {
                *(u16 *)p = c;
            } else {
                u8 r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
                r |= r >> 5; g |= g >> 6; b |= b >> 5;
                if (bpp == 3) { p[0] = b; p[1] = g; p[2] = r; }
                else          { p[0] = 0xFF; p[1] = b; p[2] = g; p[3] = r; }
            }
        }
    }
    GSPGPU_FlushDataCache(fb, UI_W * UI_H * bpp);
    // presents the buffer and, the first time, tells the LCD its pixel format
    // (the menu and the GPU background leave a different one set)
    gfxScreenSwapBuffers(GFX_BOTTOM, false);
}

void guide3dsBeginShowing()
{
    // single buffering, like the profiler's console: what is written shows directly.
    // RGB565, the canvas' own format, so the copy needs no conversion.
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    if (gfxGetScreenFormat(GFX_BOTTOM) != GSP_RGB565_OES)
        gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB565_OES);
    guide_draw();
    present();
}

bool guide3dsHandleTouch(u32 keysDown, u32 keysHeld, u32 keysUp)
{
    touchPosition tp;
    int r = 0;

    if (keysDown & KEY_TOUCH) {
        hidTouchRead(&tp);
        r = guide_touch(GUIDE_TOUCH_DOWN, tp.px, tp.py);
    } else if (keysHeld & KEY_TOUCH) {
        hidTouchRead(&tp);
        r = guide_touch(GUIDE_TOUCH_HELD, tp.px, tp.py);
    } else if (keysUp & KEY_TOUCH) {
        r = guide_touch(GUIDE_TOUCH_UP, 0, 0);
    }

    if (r == GUIDE_MENU) {
        guide_save_pos();
        return true;
    }
    if (r == GUIDE_REDRAW) {
        guide_draw();
        present();
    }
    return false;
}
