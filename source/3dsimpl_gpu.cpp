
#include "snes9x.h"
#include "ppu.h"
#include "memmap.h"
#include <stdlib.h>
#include <string.h>

#include <3ds.h>
#include "3dsgpu.h"
#include "3dsimpl.h"
#include "3dsimpl_gpu.h"
#include "3dslog.h"
#include "3dssettings.h"
#include <math.h>

SGPU3DSExtended GPU3DSExt;

void gpu3dsDeallocLayers()
{   
    SLayerList *list = &GPU3DSExt.layerList;

    if (list == nullptr)
        return;

    linearFree(list->sections);
    linearFree(list->ibo);
}

void gpu3dsResetLayer(SLayer *layer) {
    layer->sectionsByTarget[TARGET_SNES_MAIN] = 0;
    layer->verticesByTarget[TARGET_SNES_MAIN] = 0;

    layer->sectionsByTarget[TARGET_SNES_SUB] = 0;
    layer->verticesByTarget[TARGET_SNES_SUB] = 0;
    
    layer->sectionsTotal = 0;
    layer->m7Tile0 = false;
}

void gpu3dsResetLayers(SLayerList *list) {
    list->verticesTotal = 0;
            
    for (int i = 0; i < LAYERS_COUNT; i++) {
        gpu3dsResetLayer(&list->layers[i]);
    }
}

// reset to initial state when loading a game
void gpu3dsResetLayerSectionLimits(SLayerList *list) {
    list->sectionsMax = 0;

    for (int i = 0; i < LAYERS_COUNT; i++) 
    {
        SLayer *layer = &list->layers[i];

        layer->sectionsOffset = list->sectionsMax;

        switch (i) {
            case LAYER_WINDOW_LR:
            case LAYER_BRIGHTNESS:
                layer->sectionsMax = 1; // always 0-1 section
                break;
            case LAYER_BACKDROP:
                layer->sectionsMax = 2; // always 0-2 sections
                break;
            default:
                layer->sectionsMax = 128; 
                break;
        }

        list->sectionsMax += layer->sectionsMax;
    }
}

void gpu3dsAdjustLayerSectionLimits(SLayerList *list) {
    u16 threshold = 16;
    u16 allowedSectionsMax = list->sectionsSizeInBytes / sizeof(SLayerSection);
    u16 newSectionsMax = 0;

    for (int i = 0; i < LAYERS_COUNT; i++) {
        SLayer *layer = &list->layers[i];

        layer->sectionsOffset = newSectionsMax;

        if (layer->sectionsSkipped) {
            layer->sectionsMax += (layer->sectionsSkipped < threshold ? threshold : layer->sectionsSkipped);
        }
        
        newSectionsMax += layer->sectionsMax;
    }

    if (newSectionsMax <= allowedSectionsMax) {
        list->sectionsMax = newSectionsMax;

        return;
    }

    // if newSectionsMax > allowedSectionsMax, we have to reduce layer->sectionsMax for !layer->sectionsSkipped layers
    // so that newSectionsMax still fits into the allocated memory
    // (e.g. color math layer has sectionsMax = 128 but only 7 sections are needed for the current frame)
    //
    // for convinience we only do this for color math,obj and bg0-bg3 in prioritized order
    // because those layers will have the highest number of unused sections
    u16 sectionsToReduce = newSectionsMax - allowedSectionsMax;
    u16 reducedSections = 0;

    LAYER_ID order[6] = {
        LAYER_COLOR_MATH,
        LAYER_OBJ,
        LAYER_BG3,
        LAYER_BG2,
        LAYER_BG1,
        LAYER_BG0,
    };

    for (int i = 0; i < 6; i++) {
        SLayer *layer = &list->layers[order[i]];

        if (!layer->sectionsSkipped) {
            u16 newMax = layer->sectionsTotal < threshold ? threshold : layer->sectionsTotal;
            reducedSections += layer->sectionsMax - newMax;
            layer->sectionsMax = newMax;
        }

        if (sectionsToReduce <= reducedSections)
            break;
    }

    newSectionsMax = 0;
    for (int i = 0; i < LAYERS_COUNT; i++) {
        SLayer *layer = &list->layers[i];

        // just in case if we still exceed the max limit (should never happen)
        if (newSectionsMax + layer->sectionsMax > allowedSectionsMax) {
            layer->sectionsMax = 0;
        }

        layer->sectionsOffset = newSectionsMax;

        if (layer->sectionsSkipped) {
            layer->sectionsSkipped = false;
        }

        newSectionsMax += layer->sectionsMax;
    }

    list->sectionsMax = newSectionsMax;
}

u64 gpu3dsGetLayerPackedMask(LAYER_ID id, bool firstSection) {
    if (id == LAYER_OBJ)
    {
        return firstSection
            ? PACKED_MASK_TEX_BIND | PACKED_MASK_STENCIL | PACKED_MASK_ALPHA_TEST | PACKED_MASK_TEX_OFFSET
            : PACKED_MASK_STENCIL;
    }

    if (id == LAYER_BG0 || id == LAYER_BG1)
    {
        return PACKED_MASK_TEX_BIND
            | PACKED_MASK_STENCIL
            | PACKED_MASK_ALPHA_TEST
            | PACKED_MASK_TEX_OFFSET;
    }

    if (id == LAYER_BG2 || id == LAYER_BG3)
    {
        return firstSection
            ? PACKED_MASK_TEX_BIND | PACKED_MASK_STENCIL | PACKED_MASK_ALPHA_TEST | PACKED_MASK_TEX_OFFSET
            : PACKED_MASK_STENCIL;
    }

    return 0;
}

void gpu3dsInitLayers() {
    SLayerList *list = &GPU3DSExt.layerList;

    // Sized by index references, not unique vertices.
    // A tile drawn on both sub and main is referenced twice, so worst case is 2x MAX_VERTICES.
    list->sizeInBytes = gpu3dsGetNextPowerOf2(2 * MAX_VERTICES * sizeof(u16));
    list->ibo = linearAlloc(list->sizeInBytes);

    gpu3dsResetLayers(list);

    for (int i = 0; i < LAYERS_COUNT; i++) 
    {
        LAYER_ID id = LAYER_ID(i);
        SLayer *layer = &list->layers[id];

        layer->id = id;
    }

    list->sectionsSizeInBytes = gpu3dsGetNextPowerOf2(list->sectionsMax * sizeof(SLayerSection));
    list->sections = (SLayerSection *)linearAlloc(list->sectionsSizeInBytes);
			
    log3dsWrite("ibo size: %dkb, sections size: %dkb",
        list->sizeInBytes / 1024,
        list->sectionsSizeInBytes / 1024
    );
}

int compareSections(const SLayerSection *a, const SLayerSection *b, bool tile0) {
    // First, compare target: TARGET_SNES_SUB should come first
    if (a->onSub != b->onSub) {
        return a->onSub ? -1 : 1;
    }
    
    // If targets are equal, compare texture: SNES_MODE7_TILE_0 should come first
    if (tile0 && a->state.textureBind != b->state.textureBind) {
        return (a->state.textureBind == SNES_MODE7_TILE_0) ? -1 : 1;
    }

    return 0;
}

void sortSections(SLayerSection *sections, int n, bool tile0) {
    for (int i = 1; i < n; i++) {
        SLayerSection section = sections[i];
        int j = i - 1;
        while (j >= 0 && compareSections(&section, &sections[j], tile0) < 0) {
            sections[j + 1] = sections[j];
            j--;
        }
        sections[j + 1] = section;
    }
}

// Menu layer highlight (Settings > 3D Layer Depth, Show Only Selected Layer unchecked): while true, the tile
// layers are drawn with TEX_ENV_REPLACE_TEXTURE0_TINT_COLOR_ALPHA and gpu3dsLayerTintColor.
static bool stereoTintActive = false;

static inline SGPU_TEX_ENV layerTextureEnv() {
    return stereoTintActive ? TEX_ENV_REPLACE_TEXTURE0_TINT_COLOR_ALPHA : TEX_ENV_REPLACE_TEXTURE0_COLOR_ALPHA;
}

// window_lr, backdrop, color math, brightness
void gpu3dsDrawVerticalSectionLayer(SLayer *layer, int from, int to) {
    SLayerList *list = &GPU3DSExt.layerList;

    u64 mask = PACKED_MASK_TEX_ENV
        | PACKED_MASK_STENCIL
        | PACKED_MASK_ALPHA_TEST
        | PACKED_MASK_ALPHA_BLEND;

    if (layer->id == LAYER_COLOR_MATH)
        mask |= PACKED_MASK_TEX_BIND;

    for (int i = from; i < to; i++) {
        SLayerSection *section = &list->sections[i];

        GPU3DS.currentRenderState.packed =
            (GPU3DS.currentRenderState.packed & ~mask) | (section->state.packed & mask);
        gpu3dsDraw(&GPU3DS.vertices[section->vboId], NULL, section->count, section->from);
    }
}

// obj, bg0-bg3 - single section per target.
// Only used when list->useDrawArraysForTiledLayers holds, because mixing
// DrawArrays and DrawElements on the OBJ/BG pass seems to freeze real hardware
// (SMK race).
void gpu3dsDrawTiledLayerSingleSection(SLayer *layer, SLayerSection *section) {
    GPU3DS.currentRenderState.textureEnv = layerTextureEnv();
    GPU3DS.currentRenderState.alphaBlending = ALPHA_BLENDING_DISABLED;

    u64 mask = gpu3dsGetLayerPackedMask(layer->id, true);
    GPU3DS.currentRenderState.packed =
        (GPU3DS.currentRenderState.packed & ~mask) | (section->state.packed & mask);

    gpu3dsDraw(&GPU3DS.vertices[section->vboId], NULL, section->count, section->from);
}

// obj, bg0-bg3
void gpu3dsDrawTiledLayer(SLayer *layer, u16 *indices, int from, int to) {
    SLayerList *list = &GPU3DSExt.layerList;
    u16 batchFrom = 0;
    u16 batchCount = 0;

    // Those fields are set once before the loop and stay constant
    GPU3DS.currentRenderState.textureEnv = layerTextureEnv();
    GPU3DS.currentRenderState.alphaBlending = ALPHA_BLENDING_DISABLED;
    
    // restrict the diff to only the properties that actually vary across sections for that layer type
    u64 layerMask[2];
    layerMask[0] = gpu3dsGetLayerPackedMask(layer->id, true);  // first section
    layerMask[1] = gpu3dsGetLayerPackedMask(layer->id, false); // subsequent sections

    // init from first section
    SLayerSection *first = &list->sections[from];
    SGPU_VBO_ID vboId = first->vboId;
    GPU3DS.currentRenderState.packed =
        (GPU3DS.currentRenderState.packed & ~layerMask[0]) | (first->state.packed & layerMask[0]);

    for (int idx = from; idx < to; idx++) {
        SLayerSection *section = &list->sections[idx];
        u16 sFrom = section->from;
        u16 sCount = section->count;

        // batch break on state change
        if (idx > from) {
            bool changed = ((GPU3DS.currentRenderState.packed ^ section->state.packed) & layerMask[1])
                || section->vboId != vboId;

            if (changed) {
                gpu3dsDraw(&GPU3DS.vertices[vboId], (void *)(indices + batchFrom), batchCount);
                vboId = section->vboId;
                GPU3DS.currentRenderState.packed =
                    (GPU3DS.currentRenderState.packed & ~layerMask[1]) | (section->state.packed & layerMask[1]);
                batchFrom += batchCount;
                batchCount = 0;
            }
        }

        // build sequential indices
        u16 *dst = indices + batchFrom + batchCount;
        int i = 0;
        for (; i <= sCount - 4; i += 4) {
            dst[i]     = sFrom + i;
            dst[i + 1] = sFrom + i + 1;
            dst[i + 2] = sFrom + i + 2;
            dst[i + 3] = sFrom + i + 3;
        }
        for (; i < sCount; i++) {
            dst[i] = sFrom + i;
        }

        batchCount += sCount;
    }

    // draw final batch
    gpu3dsDraw(&GPU3DS.vertices[vboId], (void *)(indices + batchFrom), batchCount);
}

static bool stereoLayersPending = false;
static int  stereoEye = 0; // -1 = left eye pass, +1 = right eye pass, 0 = normal single pass

//---------------------------------------------------------
// Per-layer 3D depth modes (Settings: "Depth Mode")
//   Manual       the five depth sliders
//   Recommended  a depth list for known games (by ROM CRC32), otherwise a general preset
//   Auto         estimates each layer's depth from how fast it scrolls (parallax): the fastest
//                layer is the play field, slower layers are farther away, a layer that never
//                scrolls is the HUD (BG3) or a fixed backdrop (farthest). While nothing scrolls
//                (menus, name entry, title screens) it goes back to the Recommended depths.
// All depths are BG1..BG4, OBJ in SNES pixels at full slider, -8..8.
//---------------------------------------------------------
struct StereoGameDepths {
    u32 crc32;
    s8  depth[5];
};

static const StereoGameDepths stereoGameTable[] = {
    { 0xEAC3358D, { -1, -4,  3, -2, 0 } },   // ActRaiser (USA)
    // BG1 and sprites at the same depth so the characters do not float above the ground,
    // BG2 -4 keeps the mountain layer from looking flat (CRC32s and depths from SWChris on r/3dshomebrew)
    { 0xD138F224, {  0, -4,  3, -2, 0 } },   // Yoshi's Island
    { 0xB19ED489, {  0, -4,  3, -2, 0 } },   // Super Mario World

    // More games, CRC32s and depths from SWChris (r/3dshomebrew and the project Discord)
    { 0x925637C7, {  0, -4, -8,  0, 0 } },   // Super Mario All-Stars (the Super Mario Bros 1/2 and Lost Levels depths,
                                             //   they cover the most games; Super Mario Bros 3 looks best at BG2 -8, BG3 0)
    { 0xCD80DB86, {  0,  2, -4, -8, 0 } },   // Super Mario Kart
    { 0x231F0F67, { -3, -3,  3,  0, 0 } },   // UN Squadron (foreground elements share a layer with the background, so they look inverted)
    { 0x383858C7, {  0, -8,  3,  0, 0 } },   // Uniracers
    { 0x468D8CD0, { -1, -1,  3,  0, 0 } },   // Bill Laimbeer's Combat Basketball
    { 0x3A4A47EB, {  0, -4, -8,  0, 1 } },   // Earthworm Jim
    { 0x393DE197, {  0, -4, -8,  0, 0 } },   // Earthworm Jim 2 (BG3 priority flips in the Lorenzo's Soil and ISO 9000 levels)
    { 0x124D8E4D, { -1, -3,  3,  0, 0 } },   // Aladdin
    { 0x4AAFA462, {  1,  0,  3,  0, 1 } },   // Goof Troop
    { 0x8FC4E6D0, {  0, -4,  3,  0, 0 } },   // Star Fox
    { 0x87653732, { -5,  0,  0,  0, 2 } },   // Star Fox 2 (also starts with Crop Top/Bottom Scanlines 0, see 3dssettings.cpp)
    { 0xE51A3FCD, { -2, -5,  0,  0, 0 } },   // Mario is Missing!

    // The Legend of Zelda: A Link to the Past changes which layer Link walks on from room to room, so no single
    // setting fits every scene. This is SWChris's setting for open scenes (no layers Link can walk under); rooms with
    // bridges, pits or stairs need other depths (see STEREO_LAYERS.md, Game notes).
    { 0x777AAC2F, {  0,  0,  1,  0, 0 } },   // The Legend of Zelda: A Link to the Past
};

// General preset, checked by hand on Donkey Kong Country, Secret of Mana and Super Metroid.
// BG1 sits 1 step behind the sprites so characters pop out of the play field. Games where characters
// look like they float can be set to BG1 0 in Manual, or get their own line in stereoGameTable.
static const s8 stereoPreset[5] = { -1, -3, 3, -2, 0 };

static void stereoGetRecommended(s8 out[5])
{
    for (size_t i = 0; i < sizeof(stereoGameTable) / sizeof(stereoGameTable[0]); i++) {
        if (stereoGameTable[i].crc32 == Memory.ROMCRC32) {
            memcpy(out, stereoGameTable[i].depth, 5);
            return;
        }
    }

    memcpy(out, stereoPreset, 5);
}

static struct {
    int  lastMode;
    u32  lastCrc;
    bool havePrev;
    u16  prevH[4];
    u16  prevV[4];
    float speed[4];     // smoothed scroll speed, pixels per frame
    float peak[4];      // slowly decaying maximum of speed: a layer's typical scroll speed
    int  current[5];    // depths in use
    int  target[5];     // depths Auto wants to reach
    int  frame;
} stereoAuto = { -1, 0, false };

static void stereoAutoReset(int mode)
{
    s8 rec[5];
    stereoGetRecommended(rec);

    memset(&stereoAuto, 0, sizeof(stereoAuto));
    stereoAuto.lastMode = mode;
    stereoAuto.lastCrc = Memory.ROMCRC32;

    // every mode starts from the Recommended depths; Auto moves away from them once layers scroll
    for (int i = 0; i < 5; i++) {
        stereoAuto.current[i] = rec[i];
        stereoAuto.target[i] = rec[i];
    }
}

static inline int stereoScrollDelta(u16 now, u16 before)
{
    int d = (now - before) & 0x3FF;     // BG scroll registers are 10 bits and wrap
    return d > 512 ? d - 1024 : d;
}

static inline int stereoClampDepth(int d)
{
    return d < -8 ? -8 : (d > 8 ? 8 : d);
}

// Called once per drawn frame, before the layers are drawn.
static void stereoUpdateDepths(SLayerList *list)
{
    int mode = settings3DS.StereoDepthMode;

    if (mode != stereoAuto.lastMode || Memory.ROMCRC32 != stereoAuto.lastCrc)
        stereoAutoReset(mode);

    if (mode == STEREO_DEPTH_MANUAL)
        return;

    s8 rec[5];
    stereoGetRecommended(rec);

    if (mode == STEREO_DEPTH_RECOMMENDED) {
        for (int i = 0; i < 5; i++)
            stereoAuto.current[i] = rec[i];
        return;
    }

    // ---- Auto ----
    // the Recommended depths unless layers are scrolling (menus, title screens and static scenes go back to them)
    for (int i = 0; i < 5; i++)
        stereoAuto.target[i] = rec[i];

    bool present[4];
    for (int n = 0; n < 4; n++) {
        SLayer *layer = &list->layers[LAYER_BG0 + n];
        present[n] = (layer->verticesByTarget[TARGET_SNES_MAIN] + layer->verticesByTarget[TARGET_SNES_SUB]) > 0;
    }

    // sample the scroll positions
    for (int n = 0; n < 4; n++) {
        u16 h = PPU.BG[n].HOffset & 0x3FF;
        u16 v = PPU.BG[n].VOffset & 0x3FF;

        if (stereoAuto.havePrev) {
            int dx = abs(stereoScrollDelta(h, stereoAuto.prevH[n]));
            int dy = abs(stereoScrollDelta(v, stereoAuto.prevV[n]));
            float s = (float)(dx + dy);

            // a jump of this size is a scene change or a screen effect, not scrolling
            if (s > 64.0f)
                s = 0.0f;

            stereoAuto.speed[n] += (s - stereoAuto.speed[n]) * (1.0f / 16.0f);

            float decayed = stereoAuto.peak[n] * 0.9996f;
            stereoAuto.peak[n] = stereoAuto.speed[n] > decayed ? stereoAuto.speed[n] : decayed;
        }

        stereoAuto.prevH[n] = h;
        stereoAuto.prevV[n] = v;
    }
    stereoAuto.havePrev = true;

    // the fastest layer is the play field
    int ref = -1;
    float refPeak = 0.0f;

    if (PPU.BGMode != 7) {
        for (int n = 0; n < 4; n++) {
            if (present[n] && stereoAuto.peak[n] > refPeak) {
                refPeak = stereoAuto.peak[n];
                ref = n;
            }
        }
    }

    // until something has really scrolled there is nothing to measure: stay on the Recommended depths
    if (ref >= 0 && refPeak >= 0.75f) {
        const int playDepth = -1;

        for (int n = 0; n < 4; n++) {
            if (!present[n])
                continue;

            float r = stereoAuto.peak[n] / refPeak;
            int depth;

            if (r < 0.08f)
                depth = (n == 2) ? 3 : -6;                      // never scrolls: HUD (BG3) or a fixed backdrop
            else
                depth = playDepth - (int)lroundf(5.0f * (1.0f - r));   // slower than the play field = farther away

            stereoAuto.target[n] = stereoClampDepth(depth);
        }

        stereoAuto.target[4] = stereoClampDepth(stereoAuto.target[ref] + 1);   // sprites ride with the play field
    }

    // move toward the target one step at a time so the picture never jumps
    if (++stereoAuto.frame >= 8) {
        stereoAuto.frame = 0;

        for (int i = 0; i < 5; i++) {
            if (stereoAuto.current[i] < stereoAuto.target[i]) stereoAuto.current[i]++;
            else if (stereoAuto.current[i] > stereoAuto.target[i]) stereoAuto.current[i]--;
        }
    }
}

// The depths Recommended mode uses for the current game (the list entry, or the general preset).
void gpu3dsStereoGetRecommended(int out[5])
{
    s8 rec[5];
    stereoGetRecommended(rec);

    for (int i = 0; i < 5; i++)
        out[i] = rec[i];
}

// The depths the renderer uses right now (Manual: the sliders).
void gpu3dsStereoGetDepths(int out[5])
{
    for (int i = 0; i < 5; i++)
        out[i] = settings3DS.StereoDepthMode == STEREO_DEPTH_MANUAL
                     ? settings3DS.StereoLayerDepth[i]
                     : stereoAuto.current[i];
}

// One eye's horizontal shift, in whole SNES pixels, for a layer depth. The disparity between the eyes
// is a whole number of SNES pixels so tiles stay on the pixel grid; an odd pixel goes to the left eye.
static int stereoDepthShift(int depth, bool rightEye)
{
    float disparity = depth * osGet3DSliderState() * (gpu3dsGetIODBase() / 3.0f);
    int d = (int)lroundf(disparity);

    return rightEye ? -(int)floorf(d / 2.0f) : (int)ceilf(d / 2.0f);
}

// Horizontal shift (in texture pixels) for a layer in one eye.
static float stereoLayerShift(LAYER_ID id, bool rightEye)
{
    if (id > LAYER_OBJ)
        return 0.0f;

    int depths[5];
    gpu3dsStereoGetDepths(depths);

    return (float)(stereoDepthShift(depths[id], rightEye) * (GPU3DSExt.renderWidth / 256));
}

// Mode 7 lines of the frame emulated last (the one drawn next): the first and last line and the
// smallest / largest squared horizontal scale (M7A^2 + M7C^2; lines where it is 0 are left out).
static struct {
    int   top, bottom;
    float minScale2, maxScale2;
} stereoMode7 = { 1000, -1, 0.0f, 0.0f };
static bool stereoMode7PrevFrame;   // the frame emulated before that had Mode 7 lines

// Whether the Mode 7 builder should report its lines (Per-Layer 3D on, slider up, tilt above 0).
bool gpu3dsStereoTiltActive()
{
    return settings3DS.Stereo3DLayers && settings3DS.StereoMode7Tilt > 0 && gpu3dsGetIOD() > 0.0f;
}

// Called before emulating a frame that is going to be shown (skipped frames keep the last stats).
void gpu3dsStereoBeginEmulatedFrame()
{
    stereoMode7PrevFrame = stereoMode7.bottom >= 0;
    stereoMode7.top = 1000;
    stereoMode7.bottom = -1;
    stereoMode7.minScale2 = 0.0f;
    stereoMode7.maxScale2 = 0.0f;
}

void gpu3dsStereoNoteMode7Line(int y, int a, int c)
{
    if (y < stereoMode7.top) stereoMode7.top = y;
    if (y > stereoMode7.bottom) stereoMode7.bottom = y;

    float scale2 = (float)a * a + (float)c * c;
    if (scale2 > 0.0f) {
        if (stereoMode7.minScale2 == 0.0f || scale2 < stereoMode7.minScale2) stereoMode7.minScale2 = scale2;
        if (scale2 > stereoMode7.maxScale2) stereoMode7.maxScale2 = scale2;
    }
}

// Floor tilt for one eye of the frame being drawn (the stereoTilt uniform of shader_tiles.g.pica).
// A perspective floor's disparity falls linearly from its far (top) line to its near (bottom) line,
// so tiles get an extra shift of `tilt` depth steps back at the floor's top line and above, fading to
// none at its bottom line. The strength follows how much the Mode 7 scale changes down the floor:
// none for flat Mode 7 (one scale on every line: rotating maps, bosses), all of it once the far line's
// scale is twice the near line's, and in between it fades, so a game zooming into a floor view
// doesn't jump. Returns false (no tilt) when there is no such floor.
static bool stereoTiltParams(bool rightEye, float out[4])
{
    if (!gpu3dsStereoTiltActive() || stereoMode7.bottom < 0 || stereoMode7.minScale2 <= 0.0f)
        return false;

    float strength = sqrtf(stereoMode7.maxScale2 / stereoMode7.minScale2) - 1.0f;
    if (strength <= 0.0f)
        return false;
    if (strength > 1.0f)
        strength = 1.0f;

    // total separation at full tilt, in render-target pixels (negative = further away), split
    // between the eyes like the layer depths
    float separation = -strength * settings3DS.StereoMode7Tilt * osGet3DSliderState() * (gpu3dsGetIODBase() / 3.0f)
                     * (GPU3DSExt.renderWidth / 256);
    int span = stereoMode7.bottom > stereoMode7.top ? stereoMode7.bottom - stereoMode7.top : 1;

    out[0] = (rightEye ? -0.5f : 0.5f) * separation;
    out[1] = (float)stereoMode7.bottom;
    out[2] = 1.0f / span;
    out[3] = 0.0f;
    return true;
}

// Extra 8 pixel tile columns the layer builders add past each screen edge. A shifted layer would
// otherwise leave an empty strip as wide as its shift at one edge; with the extra columns the tiles
// that move in come from the real tilemap. Sized for the widest shift of any layer in either eye
// (Auto also counts the depth it is moving toward), and 0 when 3D is off or the slider is at zero,
// so then the frame is built exactly as before.
int gpu3dsStereoEdgeTiles()
{
    if (!settings3DS.Stereo3DLayers || gpu3dsGetIOD() <= 0.0f)
        return 0;

    int widest = 0;

    for (int id = LAYER_BG0; id <= LAYER_OBJ; id++) {
        int depth = settings3DS.StereoDepthMode == STEREO_DEPTH_MANUAL
                        ? settings3DS.StereoLayerDepth[id]
                        : stereoAuto.current[id];
        int other = settings3DS.StereoDepthMode == STEREO_DEPTH_MANUAL ? depth : stereoAuto.target[id];

        // with a Mode 7 floor about, every layer may also be moved back by up to the floor tilt
        int tilt = gpu3dsStereoTiltActive() && (stereoMode7PrevFrame || PPU.BGMode == 7) ? settings3DS.StereoMode7Tilt : 0;

        for (int eye = 0; eye < 2; eye++) {
            int a = abs(stereoDepthShift(depth, eye != 0));
            int b = abs(stereoDepthShift(other, eye != 0));
            int c = abs(stereoDepthShift(depth - tilt, eye != 0));
            int d = abs(stereoDepthShift(other - tilt, eye != 0));

            if (a > widest) widest = a;
            if (b > widest) widest = b;
            if (c > widest) widest = c;
            if (d > widest) widest = d;
        }
    }

    return (widest + 7) / 8;
}

static void stereoSetLayerShift(float shift)
{
    gpu3dsSetStereoShift(shift);
}

// Menu highlight colours for BG1, BG2, BG3, BG4, sprites: red, blue, yellow, pink, green (the colours named on the
// Settings sliders), as 0xRRGGBB. The selected layer gets its colour strongly; the others a pale version of theirs.
// The TEV doubles texture * constant, so the constant is half of the wanted multiplier.
static u32 stereoLayerTintColor(LAYER_ID id, bool selected)
{
    static const u32 rgb[5] = { 0xFF2828, 0x2878FF, 0xFFFF28, 0xFF28FF, 0x28FF28 };
    u32 c = rgb[id <= LAYER_OBJ ? id : 0];
    u32 channel[3] = { (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF };

    for (int k = 0; k < 3; k++)
        channel[k] = selected ? channel[k] : (153 + channel[k] * 2 / 5) / 2;   // pale: 60% white + 40% of the colour, halved

    return 0xFF000000u | (channel[2] << 16) | (channel[1] << 8) | channel[0];   // 0xAABBGGRR
}

void gpu3dsDrawLayers(SLayerList *list) {
    // draw window_lr into depth buffer first
    SLayer *layer = &list->layers[LAYER_WINDOW_LR];

    if (layer->verticesByTarget[0]) {
        GPU3DS.currentRenderState.target = TARGET_SNES_DEPTH;

        gpu3dsDrawVerticalSectionLayer(layer, layer->sectionsOffset, layer->sectionsOffset + layer->sectionsByTarget[TARGET_SNES_MAIN]);
    }

    u8 i0 = list->anythingOnSub ? 1 : 0;

    for (int i = i0; i >= 0; i--) {
        GPU3DS.currentRenderState.target = (SGPU_TARGET_ID)i;

        bool sub = i == TARGET_SNES_SUB;

        for (int j = 0; j < list->layersTotalByTarget[i]; j++) {
            LAYER_ID id = list->layersByTarget[i][j];
            SLayer *layer = &list->layers[id];

            int from = layer->sectionsOffset + (sub ? 0 : layer->sectionsByTarget[TARGET_SNES_SUB]);
            int to = from + layer->sectionsByTarget[i];

            if (to <= from) continue;

            // menu preview of one layer (Settings > 3D Layer Depth): skip the other BGs / sprites
            if (id <= LAYER_OBJ && settings3DS.StereoSoloLayer >= 0 && id != (LAYER_ID)settings3DS.StereoSoloLayer)
                continue;

            GPU3DS.currentRenderState.depthTest = id < LAYER_OBJ ? SGPU_STATE_ENABLED : SGPU_STATE_DISABLED;

            // menu preview: the selected layer in a strong colour, the others in a pale one
            stereoTintActive = id <= LAYER_OBJ && settings3DS.StereoHighlightLayer >= 0 && settings3DS.StereoSoloLayer < 0;
            if (stereoTintActive) {
                gpu3dsLayerTintColor = stereoLayerTintColor(id, id == (LAYER_ID)settings3DS.StereoHighlightLayer);
                GPU3DS.appliedRenderState.textureEnv = TEX_ENV_UNSET;   // the tint colour changes per layer: re-apply the environment
            }

            if (id < LAYER_BACKDROP)
                stereoSetLayerShift(stereoEye != 0 ? stereoLayerShift(id, stereoEye > 0) : 0.0f);

            if (id < LAYER_BACKDROP) {
                if (list->useDrawArraysForTiledLayers) {
                    gpu3dsDrawTiledLayerSingleSection(layer, &list->sections[from]);
                }
                else {
                    u32 bufferOffset = layer->bufferOffset + (sub ? 0 : layer->verticesByTarget[TARGET_SNES_SUB]);
                    u16 *indices = (u16 *)list->ibo + bufferOffset;
                    gpu3dsDrawTiledLayer(layer, indices, from, to);
                }
            }
            else {
                gpu3dsDrawVerticalSectionLayer(layer, from, to);
            }
        }
    }

    stereoTintActive = false;
}

void gpu3dsDrawMode7Texture()
{
    if (!IPPU.Mode7Prepared || !GPU3DSExt.mode7TilesModified) return;

	t3dsStartTimer(TIMER_DRAW_M7_TEXTURE);
	gpu3dsSetMode7TexturesPixelFormat(IPPU.Mode7EXTBGFlag ? GPU_RGBA4 : GPU_RGBA5551);

	GPU3DS.currentRenderState.textureBind = SNES_MODE7_TILE_CACHE;
	GPU3DS.currentRenderState.shader = SPROGRAM_MODE7;
	GPU3DS.currentRenderState.textureEnv = TEX_ENV_REPLACE_TEXTURE0;
	GPU3DS.currentRenderState.stencilTest = STENCIL_TEST_DISABLED;
	GPU3DS.currentRenderState.alphaTest = ALPHA_TEST_DISABLED;
	GPU3DS.currentRenderState.alphaBlending = ALPHA_BLENDING_DISABLED;

	SVertexList *list = &GPU3DS.vertices[VBO_MODE7_TILE];
    SGPUTexture *texture = &GPU3DS.textures[SNES_MODE7_FULL];

    // 3DS does not allow rendering to a viewport whose width > 512
    // so our 1024x1024 texture is split into 4 512x512 parts
	GPU3DS.currentRenderState.target = TARGET_SNES_MODE7_FULL;

	for (int section = 0; section < 4; section++)
	{
		if (GPU3DSExt.mode7SectionsModified[section])
		{
			GPU3DSExt.mode7SectionsModified[section] = false;

			// invalidate so next gpu3dsApplyRenderState re-applies the target (framebuf address changes per section)
			GPU3DS.appliedRenderState.target = TARGET_UNSET;
			int addressOffset = ((3 - section) * 0x40000) * gpu3dsGetPixelSize(texture->tex.fmt);
    		texture->target->frameBuf.colorBuf = (void *)((int)texture->tex.data + addressOffset);

			gpu3dsDraw(list, NULL, 4096, 4096 * section);
		}
	}

	GPU3DSExt.mode7TilesModified = false;

	// Citra workaround: 
    // without this, drawing all 4 sections above leaves the Mode 7 texture blank on Citra.
	// Force Citra to flush the surface to memory; 16 bytes seems enough here
	if (!GPU3DS.isReal3DS)
	{
		C3D_SyncTextureCopy(
			(u32 *)texture->tex.data, 0,
			(u32 *)texture->tex.data, 0,
			16, 8);
	}

	// SNES_MODE7_TILE_0 is only sampled when Mode7Repeat is 3 ("repeat tile 0
	// across the plane"). Modes 0 and 2 (wrap / fill with colour 0) don't
	// sample it. Skipping the bake saves one draw + one render-target switch
	// every Mode 7 frame on those modes.
	if (PPU.Mode7Repeat == 3)
	{
		GPU3DS.currentRenderState.target = TARGET_SNES_MODE7_TILE_0;
		gpu3dsDraw(list, NULL, 4, 16384);
	}

	// re-bind our tile shader
	GPU3DS.currentRenderState.shader = SPROGRAM_TILES;

	t3dsStopTimer(TIMER_DRAW_M7_TEXTURE);

    gpu3dsIncrementMode7UpdateFrameCount();
}

void gpu3dsPrepareSnesScreenForNextFrame() {
    SLayerList *list = &GPU3DSExt.layerList;

    if (list->hasSkippedSections) {
        gpu3dsAdjustLayerSectionLimits(list);
        list->hasSkippedSections = false;
    }
    
    gpu3dsResetLayers(list);

    // flip snes VBOs to the alternate half of the buffer
    // make sure this is called BEFORE S9xMainLoop so that vertex writes go to different memory
	gpu3dsPrepareListForNextFrame(&GPU3DS.vertices[VBO_SCENE_RECT], true);
	gpu3dsPrepareListForNextFrame(&GPU3DS.vertices[VBO_SCENE_TILE], true);
	gpu3dsPrepareListForNextFrame(&GPU3DS.vertices[VBO_SCENE_MODE7_LINE], true);

    if (GPU3DSExt.render2x.dirty) {
        gpu3dsClearTexture(&GPU3DS.textures[SNES_MAIN], 0);
        gpu3dsClearTexture(&GPU3DS.textures[SNES_SUB], 0);
        gpu3dsClearTexture(&GPU3DS.textures[SNES_DEPTH], 0);
        GPU3DSExt.render2x.dirty = false;
    }
}

void gpu3dsDrawSnesScreen(bool allowStereo) {
    SLayerList *list = &GPU3DSExt.layerList;

    stereoLayersPending = false;

    if (!list->verticesTotal || list->hasSkippedSections)
        return;

    list->anythingOnSub = false;
    list->useDrawArraysForTiledLayers = true;
    list->layersTotalByTarget[TARGET_SNES_SUB] = 0;
    list->layersTotalByTarget[TARGET_SNES_MAIN] = 0;

    LAYER_ID drawOrder[8] = {
        LAYER_BACKDROP,
        LAYER_OBJ,
        LAYER_BG0,
        LAYER_BG1,
        LAYER_BG2,
        LAYER_BG3,
        LAYER_COLOR_MATH,
        LAYER_BRIGHTNESS,
    };

    u32 bufferOffset = 0;
    
    for (int i = 0; i < 8; i++) {
        LAYER_ID id = drawOrder[i];

        SLayer *layer = &list->layers[id];
        
        u16 verticesOnSub = layer->verticesByTarget[TARGET_SNES_SUB];
        u16 verticesOnMain = layer->verticesByTarget[TARGET_SNES_MAIN];

        int verticesTotal = verticesOnSub + verticesOnMain;

        if (!verticesTotal) {
            continue;
        }

        if (verticesOnMain) {
            list->layersByTarget[TARGET_SNES_MAIN][list->layersTotalByTarget[TARGET_SNES_MAIN]++] = id;
        }

        if (verticesOnSub) {
            list->layersByTarget[TARGET_SNES_SUB][list->layersTotalByTarget[TARGET_SNES_SUB]++] = id;
            list->anythingOnSub = true;
        }


        if ((verticesOnMain && verticesOnSub) || layer->m7Tile0) {
            sortSections(list->sections + layer->sectionsOffset, layer->sectionsTotal, layer->m7Tile0);
        }
        
        if (id < LAYER_BACKDROP)
        {
            if (layer->sectionsByTarget[TARGET_SNES_MAIN] > 1 || layer->sectionsByTarget[TARGET_SNES_SUB] > 1)
                list->useDrawArraysForTiledLayers = false;

            layer->bufferOffset = bufferOffset;
            bufferOffset += verticesTotal;
        }
    }

    if (allowStereo && settings3DS.Stereo3DLayers)
        stereoUpdateDepths(list);

    if (allowStereo && settings3DS.Stereo3DLayers && gpu3dsGetIOD() > 0.0f) {
        // the scene renderer draws the layers once per eye (gpu3dsDrawSnesScreenEye)
        stereoLayersPending = true;
        return;
    }

    gpu3dsDrawMode7Texture();
    gpu3dsDrawLayers(list);
}

bool gpu3dsSnesLayersPendingPerEye() {
    return stereoLayersPending;
}

void gpu3dsDrawSnesScreenEye(bool rightEye) {
    if (!stereoLayersPending)
        return;

    // The scene renderer left its own state behind (screen shader, screen texture, ...):
    // go back to the state a frame starts with, which is what the layer pass expects.
    gpu3dsSetDefaultRenderState(SPROGRAM_TILES, false);
    GPU3DS.currentRenderState.alphaBlending = ALPHA_BLENDING_DISABLED;

    float tilt[4];
    gpu3dsSetStereoTilt(stereoTiltParams(rightEye, tilt) ? tilt : NULL);

    stereoEye = rightEye ? 1 : -1;
    gpu3dsDrawMode7Texture();   // no-op after the first eye (it clears its own dirty flags)
    gpu3dsDrawLayers(&GPU3DSExt.layerList);
    stereoEye = 0;
    stereoSetLayerShift(0.0f);
    gpu3dsSetStereoTilt(NULL);
}

void gpu3dsEndSnesScreenEyes() {
    stereoLayersPending = false;
}

void gpu3dsCommitLayerSection(SGPU_VBO_ID vboId, LAYER_ID id, SGPURenderState *state, bool sub, bool reuseVertices) {
    SLayerList *list = &GPU3DSExt.layerList;
    SLayer *layer = &list->layers[id];

    if (layer->sectionsTotal >= layer->sectionsMax) {
        // skip current frame + count all the skipped sections 
        // to handle layer section limits for the next frame later on (gpu3dsAdjustLayerSectionLimits())
        //
        // This case should rarely happen (and never for LAYER_WINDOW_LR, LAYER_BRIGHTNESS, LAYER_BACKDROP)
        // If at all, it occurs when "In-Frame Pallete Changes" setting is set to "Enabled"
        layer->sectionsSkipped++;
        list->hasSkippedSections = true;
    }   

    int sectionIdx = layer->sectionsOffset + layer->sectionsTotal;

    if (!reuseVertices) 
    {
        SVertexList *vbo = &GPU3DS.vertices[vboId];
        u16 currentIdx = vbo->from;
        u16 currentVerticesCount = gpu3dsGetValueWithinLimit(vbo->count, list->verticesTotal, MAX_VERTICES);

        vbo->from += vbo->count;
        vbo->count = 0;

        // max sections/vertices overflow
        if (list->hasSkippedSections || !currentVerticesCount) return;

        SLayerSection *section = &list->sections[sectionIdx];

        section->state = *state;
        section->from = currentIdx;
        section->count = currentVerticesCount;
        section->vboId = vboId;
        section->onSub = sub;

        if (state->textureBind == SNES_MODE7_TILE_0) {
            layer->m7Tile0 = true;
        }
        
        layer->verticesByTarget[sub] += currentVerticesCount;
        layer->sectionsByTarget[sub]++;
        layer->sectionsTotal++;
        
        list->verticesTotal += currentVerticesCount;
    
        return;
    }

    int prevSectionIndex = sectionIdx - 1;

    if (prevSectionIndex >= 0 && !list->hasSkippedSections)
    {
        SLayerSection *section = &list->sections[sectionIdx]; // reuse last section properties

        *section = list->sections[prevSectionIndex];
        
        if (!section->count) return;

        section->state = *state;
        section->onSub = false; // reuse only happens on main
        
        layer->sectionsByTarget[TARGET_SNES_MAIN]++;
        layer->verticesByTarget[TARGET_SNES_MAIN] += section->count;
        layer->sectionsTotal++;
    }
}

void gpu3dsInitializeMode7Vertex(int idx, s16 x, s16 y)
{
    s16 x0 = 0;
    s16 y0 = 0;

    if (x < 64)
    {
        x0 = x * 8;
        y0 = (y * 2 + 1) * 8;
    }
    else
    {
        x0 = (x - 64) * 8;
        y0 = (y * 2) * 8;
    }

    SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.vertices[VBO_MODE7_TILE].data) [idx];

    m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
}

void gpu3dsInitializeMode7VertexForTile0(int idx, s16 x, s16 y)
{
    s16 x0 = x;
    s16 y0 = y;

    SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.vertices[VBO_MODE7_TILE].data) [idx];
    
    m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
}

void gpu3dsInitializeMode7Vertexes()
{
    GPU3DSExt.mode7FrameCount = 3;
    for (int f = 0; f < 2; f++)
    {
        int idx = 0;
        for (int section = 0; section < 4; section++)
        {
            for (int y = 0; y < 32; y++)
                for (int x = 0; x < 128; x++)
                    gpu3dsInitializeMode7Vertex(idx++, x, y);
        }

        gpu3dsInitializeMode7VertexForTile0(16384, 0, 0);
        gpu3dsInitializeMode7VertexForTile0(16385, 0, 8);
        gpu3dsInitializeMode7VertexForTile0(16386, 8, 0);
        gpu3dsInitializeMode7VertexForTile0(16387, 8, 8);

        gpu3dsPrepareListForNextFrame(&GPU3DS.vertices[VBO_MODE7_TILE], true);
    }

	gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
}

// Changes the texture pixel format (but it must be the same 
// size as the original pixel format). No errors will be thrown
// if the format is incorrect.
//

void gpu3dsSetMode7TexturesPixelFormat(GPU_TEXCOLOR fmt)
{
    if (GPU3DSExt.mode7TextureFormat == fmt)
        return;

    GPU3DSExt.mode7TextureFormat = fmt;
    GPU3DS.textures[SNES_MODE7_FULL].tex.fmt = fmt;
    GPU3DS.textures[SNES_MODE7_TILE_0].tex.fmt = fmt;
    GPU3DS.textures[SNES_MODE7_TILE_CACHE].tex.fmt = fmt;

    GPU_COLORBUF colorFmt = (GPU_COLORBUF)gpu3dsGetFrameBufferFmt(fmt);
    GPU3DS.textures[SNES_MODE7_FULL].target->frameBuf.colorFmt = colorFmt;
    GPU3DS.textures[SNES_MODE7_TILE_0].target->frameBuf.colorFmt = colorFmt;
}

void gpu3dsCopyVRAMTilesIntoMode7TileVertexes(uint8 *VRAM)
{
    for (int i = 0; i < 16384; i++)
    {
        gpu3dsSetMode7TileModified(i, VRAM[i * 2]);
    }
    IPPU.Mode7CharDirtyFlagCount = 1;
    for (int i = 0; i < 256; i++)
    {
        IPPU.Mode7CharDirtyFlag[i] = 2;
    }
}

void gpu3dsIncrementMode7UpdateFrameCount()
{
    gpu3dsPrepareListForNextFrame(&GPU3DS.vertices[VBO_MODE7_TILE], true);
    GPU3DSExt.mode7FrameCount ++;

    if (GPU3DSExt.mode7FrameCount == 0x3fff)
    {
        GPU3DSExt.mode7FrameCount = 1;
    }

    // Bug fix: Clears the updateFrameCount of both sets
    // of mode7TileVertexes!
    //
    if (GPU3DSExt.mode7FrameCount <= 2)
    {
        SMode7TileVertex* vertices = (SMode7TileVertex *)GPU3DS.vertices[VBO_MODE7_TILE].data;

        for (int i = 0; i < 16384; )
        {
            vertices[i++].Position.w = -1;
            vertices[i++].Position.w = -1;
            vertices[i++].Position.w = -1;
            vertices[i++].Position.w = -1;

            vertices[i++].Position.w = -1;
            vertices[i++].Position.w = -1;
            vertices[i++].Position.w = -1;
            vertices[i++].Position.w = -1;
        }
    }
}

void gpu3dsAddQuadRect(float x0, float y0, float x1, float y1, u16 wx, u16 wy, int z, u32 fillColor, u32 borderColor, u8 borderSize) 
{
    if (borderSize > 0) {
        float cx0 = x0 + borderSize;
        float cy0 = y0 + borderSize;
        float cx1 = x1 - borderSize;
        float cy1 = y1 - borderSize;
        
        // top, bottom left, right
        gpu3dsAddSimpleQuadVertexes(x0, y0, x1, cy0, wx, wy, wx, wy, z, borderColor);
        gpu3dsAddSimpleQuadVertexes(x0, cy1, x1, y1, wx, wy, wx, wy, z, borderColor);
        gpu3dsAddSimpleQuadVertexes(x0, cy0, cx0, cy1, wx, wy, wx, wy, z, borderColor);
        gpu3dsAddSimpleQuadVertexes(cx1, cy0, x1, cy1, wx, wy, wx, wy, z, borderColor);

        gpu3dsAddSimpleQuadVertexes(cx0, cy0, cx1, cy1, wx, wy, wx, wy, z, fillColor);
    } else {
        gpu3dsAddSimpleQuadVertexes(x0, y0, x1, y1, wx, wy, wx, wy, z, fillColor);
    }
}
