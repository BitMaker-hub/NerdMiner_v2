// =============================================================================
//  Guition JC3248W535 display driver for NerdMiner v2  (v0.9.18)
// =============================================================================
//
//  Stack:
//    * Arduino_GFX_Library 1.6.0 (pinned; 1.6.1+ breaks AXS15231B init).
//    * OpenFontRender 1.2  (TTF rasterizer; bridged to Arduino_Canvas).
//    * Canonical NerdMiner artwork from src/media/images_320_170.h and
//      images_bottom_320_70.h, scaled 1.5x H both axes to fill the panel.
//
//  Architecture:
//    * Display:  Arduino_AXS15231B over QSPI -> 480x320 landscape (rotation=1).
//    * Buffers:  Arduino_Canvas framebuffer in PSRAM + a separate
//                "bg_cache" PSRAM buffer holding the rendered artwork so
//                per-field text "erase" is a sub-rect memcpy from bg_cache
//                back into the canvas. No more black erase boxes.
//    * Layout:   top art y=0..220 (was 255 pre-v0.8.2), bottom y=220..320.
//                Touch zones: top tap -> next cyclic; bottom tap -> toggle
//                POOL / FEES bottom panel.
//    * 5 screens:  Loading, Setup, MinerScreen, ClockScreen,
//                  GlobalHashScreen, PriceScreen (4 cyclic).
//    * Touch:    AXS-touch I2C @ 0x3B polled at NerdMiner's 10 Hz animate()
//                cadence with 200 ms edge debounce.
//    * Flush:    gfx->flush() only called when displayed strings have
//                changed OR every JC_MAX_FLUSH_GAP_MS (60s safety net so
//                layout edits become visible without value churn).
//    * Touch debug overlay: compile with -D JC_TOUCH_DEBUG to enable a
//                live counter strip at the bottom of the panel.

#include "displayDriver.h"

#ifdef JC3248W535_DISPLAY

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <math.h>   // v0.9.16: fabsf() for hysteresis quantizers

#include "monitor.h"
#include "drivers/storage/storage.h"
#include "mining.h"
#include "version.h"

#include "OpenFontRender.h"
#include "media/myFonts.h"
#include "media/images_320_170.h"
#include "media/images_bottom_320_70.h"

extern monitor_data mMonitor;
extern pool_data    pData;
extern void switchToNextScreen();

// ============================================================================
//  Layout constants — Option C (fill width, no margins)
// ============================================================================
#define JC_W           480
#define JC_H           320
// Panel-native portrait (CASET/RASET coordinate space).
#define JC_NATIVE_W    320
#define JC_NATIVE_H    480

#define SRC_W          320
#define SRC_TOP_H      170
#define SRC_BOT_H       70

// Top: H=1.5x, V=1.294x. Bottom: H=1.5x, V=1.428x.
#define TOP_X          0
#define TOP_Y          0
#define TOP_W          480
#define TOP_H          220

#define BOT_X          0
#define BOT_Y          220
#define BOT_W          480
#define BOT_H          100

// Source->screen coord helpers (top: 1.5x both axes).
static inline int sx     (int srcX) { return (srcX * TOP_W) / SRC_W; }       // *1.5
static inline int sy_top (int srcY) { return TOP_Y + (srcY * TOP_H) / SRC_TOP_H; } // *1.5
// Bottom: 1.5x H, ~0.93x V.
static inline int sy_bot (int srcY) { return BOT_Y + (srcY * BOT_H) / SRC_BOT_H; }

// ============================================================================
//  Hardware
// ============================================================================
static Arduino_ESP32QSPI *jc_bus   = nullptr;
static Arduino_AXS15231B *jc_panel = nullptr;
static Arduino_Canvas    *gfx      = nullptr;

// Cyclic-screen IDs
enum : int { SCR_MINER = 0, SCR_CLOCK = 1, SCR_GLOBAL = 2, SCR_PRICE = 3, SCR_SLIDESHOW = 4, SCR_COUNT = 5 };
enum : int { LOWER_POOL = 1, LOWER_FEES = 2 };
static int lowerScreen = LOWER_POOL;

// Tracks current screen for context-sensitive bottom-tap (slideshow vs pool/fees toggle).
static int jc_currentScreen = SCR_MINER;

// Forward decl so jc_pollTouch() can advance slideshow images.
static void jc_slideshowAdvance();

static void toggleBottomScreen() {
    lowerScreen = (lowerScreen == LOWER_POOL) ? LOWER_FEES : LOWER_POOL;
}

// ============================================================================
//  BACKGROUND CACHE — PSRAM copy of rendered artwork for text erase
// ============================================================================
// bg_cache (~307 KB PSRAM) holds the current artwork; regenerated only on
// combo change. Canvas is restored from it before each text overlay.
static uint16_t *bg_cache       = nullptr;

// Pre-render slabs: scaled artwork cached in PSRAM (~1 MB total) to avoid
// ~100ms per-screen-change scaling cost. Populated lazily, memcpy'd into bg_cache.
#define NUM_TOP_VARIANTS  4
#define NUM_BOT_VARIANTS  2
static uint16_t *prerender_top[NUM_TOP_VARIANTS] = {nullptr, nullptr, nullptr, nullptr};
static uint16_t *prerender_bot[NUM_BOT_VARIANTS] = {nullptr, nullptr};
static int cached_cycle = -1;
static int cached_lower = -1;

// ============================================================================
//  Flash mitigation — skip gfx->flush() when displayed values unchanged
// ============================================================================
// Each cyclic screen builds a snapshot String; flush is skipped if it matches
// the cached version. Per-screen, indexed by SCR_* IDs. Force-flush on bg change.
static String last_snapshot[SCR_COUNT];
static int    last_snapshot_cycle = -1;     // tracks (cycle,lower) at last flush
static int    last_snapshot_lower = -1;

// Force-flush safety net (60s) so layout/font edits become visible. v0.9.14:
// bumped from 5s to 60s so quantized snapshots reduce flash cadence to ~once/min.
#define JC_MAX_FLUSH_GAP_MS  60000
static uint32_t last_flush_ms[SCR_COUNT] = {0, 0, 0, 0, 0};

// Side-channel: set by jc_shouldFlush() when bg changed, consumed by jc_endOfFrameFlush().
static bool jc_last_bg_changed = false;

// v0.9.16a: backlight gating — panel push with BL off (~35ms, imperceptible).
// v0.9.17d: JC_FLUSH preserves intentional backlight-off (AlternateScreenState).
#define JC_BL_PIN 1
static bool jc_backlight_enabled = true;

static inline void jc_flushGated()
{
    if (!gfx) return;
    digitalWrite(JC_BL_PIN, LOW);
    gfx->flush();
    digitalWrite(JC_BL_PIN, jc_backlight_enabled ? HIGH : LOW);
}

#define JC_FLUSH() jc_flushGated()

// ===========================================================================
//  Snapshot quantization helpers (hysteresis-based, v0.9.16)
// ===========================================================================
// Quantize snapshot strings only — displayed values stay at full resolution.
// Hysteresis: reported value updates only when raw moves >= deadband away
// from last reported value, preventing per-tick oscillation at boundaries.

// Hashrate quantizer: deadband absorbs ~13 KH/s hardware wobble (248-261 KH/s).
#define JC_HASHRATE_DEADBAND 15.0f


static String jc_qHashrateH(const String &hr, float *state) {
    float f = hr.toFloat();
    if (f <= 0.0f) return hr;
    if (*state < -0.5f || fabsf(f - *state) >= JC_HASHRATE_DEADBAND) {
        *state = f;
    }
    int q = ((int)(*state + 0.5f) + 5) / 10 * 10;
    return String("q") + String(q);
}

// Truncate "HH:MM:SS" to "HH:MM" by dropping last 3 chars if they match ":XX".
static String jc_qTimeMin(const String &t) {
    int n = t.length();
    if (n >= 3 && t.charAt(n - 3) == ':') return t.substring(0, n - 3);
    return t;
}

// Temperature quantizer: 2°C deadband absorbs ±1°C sensor jitter.
#define JC_TEMP_DEADBAND 2.0f
static float jc_temp_state = -1000.0f;   // sentinel: force first update

static String jc_qTemp(const String &t) {
    if (t.length() == 0) return t;
    float f = t.toFloat();
    if (f == 0.0f && t.charAt(0) != '0') return t;   // non-numeric, pass through

    if (jc_temp_state < -999.0f || fabsf(f - jc_temp_state) >= JC_TEMP_DEADBAND) {
        jc_temp_state = f;
    }
    int q = (int)(jc_temp_state + (jc_temp_state >= 0 ? 0.5f : -0.5f));
    return String("t") + String(q);
}

// Per-call-site hysteresis state for hashrate quantizer (sentinel = -1000.0f).
static float jc_hr_state_miner       = -1000.0f;
static float jc_hr_state_miner_pool  = -1000.0f;
static float jc_hr_state_clock       = -1000.0f;
static float jc_hr_state_clock_pool  = -1000.0f;
static float jc_hr_state_global_pool = -1000.0f;
static float jc_hr_state_price       = -1000.0f;
static float jc_hr_state_price_pool  = -1000.0f;

// Diagnostic: log snapshot string + screen, throttled to once per JC_SNAPSHOT_LOG_MS.
#ifdef JC_LOG_SNAPSHOTS
#define JC_SNAPSHOT_LOG_MS 1000
static uint32_t jc_last_snapshot_log_ms[SCR_COUNT] = {0, 0, 0, 0, 0};
static inline void jc_logSnapshot(int scr, const String &snap) {
    uint32_t now = millis();
    if (now - jc_last_snapshot_log_ms[scr] < JC_SNAPSHOT_LOG_MS) return;
    jc_last_snapshot_log_ms[scr] = now;
    Serial.printf("[jc3248w535] SNAP scr=%d: %s\n", scr, snap.c_str());
}
#else
static inline void jc_logSnapshot(int /*scr*/, const String & /*snap*/) {}
#endif

static bool jc_shouldFlush(int scr, const String &snap)
{
#ifdef JC_NO_SNAPSHOT
    // JC_NO_SNAPSHOT: force full flush every tick (bypass all quantization).
    uint32_t now = millis();
    jc_last_bg_changed = true;
    jc_logSnapshot(scr, snap);
    last_snapshot[scr] = snap;
    last_snapshot_cycle = cached_cycle;
    last_snapshot_lower = cached_lower;
    last_flush_ms[scr] = now;
    return true;
#else
    bool bg_changed = (cached_cycle != last_snapshot_cycle) ||
                      (cached_lower != last_snapshot_lower);
    bool data_changed = (last_snapshot[scr] != snap);
    uint32_t now = millis();
    bool stale = (now - last_flush_ms[scr]) > JC_MAX_FLUSH_GAP_MS;
    jc_last_bg_changed = bg_changed;
    jc_logSnapshot(scr, snap);
    if (bg_changed || data_changed || stale) {
        last_snapshot[scr] = snap;
        last_snapshot_cycle = cached_cycle;
        last_snapshot_lower = cached_lower;
        last_flush_ms[scr] = now;
        return true;
    }
    return false;
#endif
}

static inline uint16_t *bg_pix(int x, int y) { return &bg_cache[y * JC_W + x]; }

// Allocate the bg cache (call once after PSRAM is up).
static bool jc_allocBgCache() {
    if (bg_cache) return true;
    size_t bytes = (size_t)JC_W * JC_H * sizeof(uint16_t);
    bg_cache = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!bg_cache) {
        Serial.printf("[jc3248w535] !! bg_cache PSRAM alloc failed (%u bytes)\n",
                      (unsigned)bytes);
        return false;
    }
    Serial.printf("[jc3248w535] bg_cache allocated: %u bytes in PSRAM\n",
                  (unsigned)bytes);
    return true;
}

// Restore canvas rect from bg cache (call before drawing new text).
static void jc_restoreRect(int x, int y, int w, int h)
{
    if (!gfx || !bg_cache) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > JC_W) w = JC_W - x;
    if (y + h > JC_H) h = JC_H - y;
    if (w <= 0 || h <= 0) return;

    gfx->startWrite();
    for (int yy = 0; yy < h; yy++) {
        uint16_t *src_row = bg_pix(x, y + yy);
        for (int xx = 0; xx < w; xx++) {
            gfx->writePixel(x + xx, y + yy, src_row[xx]);
        }
    }
    gfx->endWrite();
}

// ============================================================================
//  Per-slot overlay canvases (v0.9.12, dormant — see note below)
// ============================================================================
// Probe v0.4-v0.6 confirmed Arduino_Canvas secondary overlay works on this
// hardware (flush pushes only the small region). CRITICAL: must restore bg
// pixels before each text draw to avoid artifacts.
// STATUS: output offsets don't affect placement on this hardware (v0.9.16);
// infrastructure dormant pending investigation. No screen functions use it.

// Forward decl
class Arduino_Canvas;

// Slot table (24 slots; ~10 used on Miner screen).
#define JC_MAX_OVERLAY_SLOTS 24

struct JcOverlaySlot {
    int             lx, ly, lw, lh;     // logical landscape rect
    Arduino_Canvas *canvas;
};
static JcOverlaySlot jc_slots[JC_MAX_OVERLAY_SLOTS];
static int           jc_slot_count   = 0;

// Get or create overlay canvas for slot; nullptr on failure (fall back to main canvas).
static Arduino_Canvas* jc_getOrCreateSlotOverlay(int lx, int ly, int lw, int lh) {
    // Look for existing matching slot
    for (int i = 0; i < jc_slot_count; i++) {
        const JcOverlaySlot &s = jc_slots[i];
        if (s.lx == lx && s.ly == ly && s.lw == lw && s.lh == lh) {
            return s.canvas;
        }
    }
    if (jc_slot_count >= JC_MAX_OVERLAY_SLOTS) {
        Serial.println(F("[jc3248w535] !! slot table full, falling back to main canvas"));
        return nullptr;
    }
    // Native portrait dims for overlay: width=lh, height=lw (transposed for rotation 1).
    // Native origin: output_x = JC_NATIVE_W - ly - lh, output_y = lx.
    int native_x = JC_NATIVE_W - ly - lh;
    int native_y = lx;
    Arduino_Canvas *ov = new Arduino_Canvas(lh, lw, jc_panel, native_x, native_y, 0);
    if (!ov || !ov->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.printf("[jc3248w535] !! slot overlay alloc/begin FAILED for (%d,%d,%d,%d)\n",
                      lx, ly, lw, lh);
        if (ov) delete ov;
        return nullptr;
    }
    ov->setRotation(1);
    jc_slots[jc_slot_count++] = { lx, ly, lw, lh, ov };
    Serial.printf("[jc3248w535] slot[%d] overlay: logical (%d,%d,%d,%d) -> native (%d,%d,%d,%d), %u bytes\n",
                  jc_slot_count - 1, lx, ly, lw, lh, native_x, native_y, lh, lw,
                  (unsigned)(lw * lh * 2));
    return ov;
}

// Restore overlay fb from bg_cache (rotation-1 inversion: fb[ov_lx * lh + (lh-1 - ov_ly)]).
static void jc_restoreOverlayFromBgCache(Arduino_Canvas *ov,
                                          int lx, int ly, int lw, int lh) {
    if (!ov || !bg_cache) return;
    uint16_t *fb = ov->getFramebuffer();
    if (!fb) return;
    for (int ov_ly = 0; ov_ly < lh; ov_ly++) {
        uint16_t *src_row = &bg_cache[(ly + ov_ly) * JC_W + lx];
        for (int ov_lx = 0; ov_lx < lw; ov_lx++) {
            fb[ov_lx * lh + (lh - 1 - ov_ly)] = src_row[ov_lx];
        }
    }
}

// ============================================================================
//  Partial-region panel push (v0.9.6, opt-in: -D JC_USE_PARTIAL_FLUSH=1)
// ============================================================================
// No longer required for flicker — snapshot quantization (v0.9.16) fixed that.
// Retained as opt-in for further perf work. EXPERIMENTAL: coordinate transform
// unvalidated (probe v0.9); v0.8.1 had an RST-line incident with partial pushes.
// Staging buffer in PSRAM, lazily grown (max ~73 KB for clock display slot).

// End-of-frame flush. force=true on screen transitions. v0.9.16a: backlight gated.
static void jc_endOfFrameFlush(bool force)
{
#ifdef JC_USE_PARTIAL_FLUSH
    if (force) {
        if (gfx) {
            JC_FLUSH();
        }
    }
    // else: partial pushes already handled by jc_dynText / jc_dynOfr.
#else
    if (gfx) {
        JC_FLUSH();
    }
#endif
}

#ifdef JC_USE_PARTIAL_FLUSH
#warning "JC_USE_PARTIAL_FLUSH is disabled internally: using one gated full flush per frame."
// v0.9.17d: jc_pushRect replaced with full-flush fallback. The coordinate
// transform was unvalidated (probe v0.9) and caused RST-line incidents (v0.8.1).
// Retaining the define so existing build configs don't break, but it now
// falls back to a full JC_FLUSH(). Remove this block entirely in v0.10
// if/when a corrected partial push is validated.
static void jc_pushRect(int /*lx*/, int /*ly*/, int /*lw*/, int /*lh*/)
{
    JC_FLUSH();
}

static inline void jc_flushOrPushRect(int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
    JC_FLUSH();
}
#endif
// Full-flush fallback: no-op here; end-of-frame gfx->flush() does the work.
static inline void jc_flushOrPushRect(int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}



// ============================================================================
//  Image scaler — RGB565 nearest-neighbor, writes into bg_cache (not canvas)
// ============================================================================
static void jc_scaleIntoBgCache(const unsigned short *src,
                                 int srcW, int srcH,
                                 int dst_x, int dst_y, int dst_w, int dst_h)
{
    if (!bg_cache || !src) return;
    for (int dy = 0; dy < dst_h; dy++) {
        int sy_src = (dy * srcH) / dst_h;
        if (sy_src >= srcH) sy_src = srcH - 1;
        const unsigned short *row = src + (sy_src * srcW);
        uint16_t *dst_row = bg_pix(dst_x, dst_y + dy);
        for (int dx = 0; dx < dst_w; dx++) {
            int sx_src = (dx * srcW) / dst_w;
            if (sx_src >= srcW) sx_src = srcW - 1;
            dst_row[dx] = pgm_read_word(&row[sx_src]);
        }
    }
}

// Blit the entire bg_cache to the canvas (used after a bg re-render).
static void jc_blitBgCacheToCanvas()
{
    if (!gfx || !bg_cache) return;
    gfx->startWrite();
    for (int y = 0; y < JC_H; y++) {
        uint16_t *src_row = bg_pix(0, y);
        for (int x = 0; x < JC_W; x++) {
            gfx->writePixel(x, y, src_row[x]);
        }
    }
    gfx->endWrite();
}

// ============================================================================
//  Pre-render slab management — populate lazily, blit on screen change
// ============================================================================
static const unsigned short *jc_topArtFor(int cyc);     // fwd
static const unsigned short *jc_bottomArtFor(int lo);   // fwd

// Allocate + scale ONE top-slab variant on demand. Returns true on success.
static bool jc_ensureTopSlab(int cyc)
{
    if (cyc < 0 || cyc >= NUM_TOP_VARIANTS) return false;
    if (prerender_top[cyc]) return true;
    size_t bytes = (size_t)TOP_W * TOP_H * sizeof(uint16_t);
    prerender_top[cyc] = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!prerender_top[cyc]) {
        Serial.printf("[jc3248w535] !! top slab %d alloc fail (%u B)\n", cyc, (unsigned)bytes);
        return false;
    }
    const unsigned short *src = jc_topArtFor(cyc);
        // NN scale into slab buffer directly.
    for (int dy = 0; dy < TOP_H; dy++) {
        int sy_src = (dy * SRC_TOP_H) / TOP_H;
        if (sy_src >= SRC_TOP_H) sy_src = SRC_TOP_H - 1;
        const unsigned short *row = src + (sy_src * SRC_W);
        uint16_t *dst_row = &prerender_top[cyc][dy * TOP_W];
        for (int dx = 0; dx < TOP_W; dx++) {
            int sx_src = (dx * SRC_W) / TOP_W;
            if (sx_src >= SRC_W) sx_src = SRC_W - 1;
            dst_row[dx] = pgm_read_word(&row[sx_src]);
        }
    }
    Serial.printf("[jc3248w535] top slab %d ready (%u KB)\n", cyc, (unsigned)(bytes/1024));
    return true;
}

// Allocate + scale ONE bottom-slab variant on demand.
static bool jc_ensureBotSlab(int lo)
{
    // lo is 1..2 in our enums; slot 0 of array unused
    int slot = (lo == LOWER_POOL) ? 0 : 1;
    if (slot < 0 || slot >= NUM_BOT_VARIANTS) return false;
    if (prerender_bot[slot]) return true;
    size_t bytes = (size_t)BOT_W * BOT_H * sizeof(uint16_t);
    prerender_bot[slot] = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!prerender_bot[slot]) {
        Serial.printf("[jc3248w535] !! bot slab %d alloc fail (%u B)\n", slot, (unsigned)bytes);
        return false;
    }
    const unsigned short *src = jc_bottomArtFor(lo);
    for (int dy = 0; dy < BOT_H; dy++) {
        int sy_src = (dy * SRC_BOT_H) / BOT_H;
        if (sy_src >= SRC_BOT_H) sy_src = SRC_BOT_H - 1;
        const unsigned short *row = src + (sy_src * SRC_W);
        uint16_t *dst_row = &prerender_bot[slot][dy * BOT_W];
        for (int dx = 0; dx < BOT_W; dx++) {
            int sx_src = (dx * SRC_W) / BOT_W;
            if (sx_src >= SRC_W) sx_src = SRC_W - 1;
            dst_row[dx] = pgm_read_word(&row[sx_src]);
        }
    }
    Serial.printf("[jc3248w535] bot slab %d ready (%u KB)\n", slot, (unsigned)(bytes/1024));
    return true;
}

// memcpy slab into bg_cache (TOP_W == JC_W so single memcpy per row).
static void jc_blitTopSlabToBgCache(int cyc)
{
    if (!bg_cache || !prerender_top[cyc]) return;
    for (int y = 0; y < TOP_H; y++) {
        memcpy(bg_pix(TOP_X, TOP_Y + y),
               &prerender_top[cyc][y * TOP_W],
               (size_t)TOP_W * sizeof(uint16_t));
    }
}

static void jc_blitBotSlabToBgCache(int lo)
{
    int slot = (lo == LOWER_POOL) ? 0 : 1;
    if (!bg_cache || !prerender_bot[slot]) return;
    for (int y = 0; y < BOT_H; y++) {
        memcpy(bg_pix(BOT_X, BOT_Y + y),
               &prerender_bot[slot][y * BOT_W],
               (size_t)BOT_W * sizeof(uint16_t));
    }
}

// ============================================================================
//  OpenFontRender bridge
// ============================================================================
static OpenFontRender render;
static bool           render_loaded = false;

// OFR callback target: set jc_ofr_target to redirect to a different canvas.
static Arduino_GFX   *jc_ofr_target = nullptr;
static inline Arduino_GFX *jc_ofr_dest() {
    return jc_ofr_target ? jc_ofr_target : (Arduino_GFX*)gfx;
}

static void jc_ofr_drawPixel(int32_t x, int32_t y, uint16_t c) {
    Arduino_GFX *d = jc_ofr_dest();
    if (d) d->writePixel((int16_t)x, (int16_t)y, c);
}
static void jc_ofr_drawHLine(int32_t x, int32_t y, int32_t w, uint16_t c) {
    Arduino_GFX *d = jc_ofr_dest();
    if (d) d->writeFastHLine((int16_t)x, (int16_t)y, (int16_t)w, c);
}
static void jc_ofr_startWrite(void) { Arduino_GFX *d = jc_ofr_dest(); if (d) d->startWrite(); }
static void jc_ofr_endWrite(void)   { Arduino_GFX *d = jc_ofr_dest(); if (d) d->endWrite();   }

enum class JcFont { Digital, NotoBold };
static JcFont jc_currentFont = JcFont::Digital;

static void jc_loadFont(JcFont f) {
    if (f == jc_currentFont && render_loaded) return;
    render.unloadFont();
    if (f == JcFont::Digital) {
        render_loaded = (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)) == 0);
    } else {
        render_loaded = (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold)) == 0);
    }
    jc_currentFont = f;
}

// ============================================================================
//  Touch
// ============================================================================
#define AXS_MAX_TOUCH_NUMBER 1
static uint32_t jc_touchPolls   = 0;
static uint32_t jc_touchHits    = 0;
static uint16_t jc_lastRawX     = 0;
static uint16_t jc_lastRawY     = 0;
static uint16_t jc_lastTapX     = 0;
static uint16_t jc_lastTapY     = 0;
static uint32_t jc_animateCount = 0;
static bool     jc_heartbeatOn  = false;

static bool jc_readRawTouch(uint16_t &rawX, uint16_t &rawY)
{
    uint8_t data[AXS_MAX_TOUCH_NUMBER * 6 + 2] = {0};
    const uint8_t read_cmd[11] = {
        0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00,
        (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) >> 8),
        (uint8_t)((AXS_MAX_TOUCH_NUMBER * 6 + 2) & 0xff),
        0x00, 0x00, 0x00
    };
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Wire.write(read_cmd, 11);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom((uint8_t)TOUCH_I2C_ADDR, (uint8_t)sizeof(data)) != sizeof(data)) return false;
    for (size_t i = 0; i < sizeof(data); i++) data[i] = Wire.read();
    if (data[1] == 0 || data[1] > AXS_MAX_TOUCH_NUMBER) return false;
    uint16_t rx = ((data[2] & 0x0F) << 8) | data[3];
    uint16_t ry = ((data[4] & 0x0F) << 8) | data[5];
    if (rx > 500 || ry > 500) return false;
    rawX = rx; rawY = ry;
    return true;
}

static uint32_t lastTouchEdgeMs = 0;
static bool     touchHeld       = false;

#ifdef SDMMC_1BIT_FIX
static void jc_pollSdStatus(); // fwd decl (defined below, alongside SD includes)
#endif

static void jc_pollTouch()
{
#ifdef SDMMC_1BIT_FIX
    jc_pollSdStatus();  // piggyback SD poll on touch tick
#endif
#ifdef JC_NO_TOUCH_POLL
    return;  // touch polling disabled (isolation test for SDMMC pin interference)
#endif
    uint16_t rawX = 0, rawY = 0;
    bool pressedRaw = jc_readRawTouch(rawX, rawY);
    jc_touchPolls++;
    if (pressedRaw) { jc_lastRawX = rawX; jc_lastRawY = rawY; jc_touchHits++; }

    bool pressed = false;
    uint16_t tx = 0, ty = 0;
    if (pressedRaw) {
        tx = rawY;
        ty = (rawX > 320) ? 0 : (320 - rawX);
        if (tx >= JC_W) tx = JC_W - 1;
        if (ty >= JC_H) ty = JC_H - 1;
        pressed = true;
    }

    uint32_t now = millis();
    if (pressed && !touchHeld && (now - lastTouchEdgeMs) > 200) {
        touchHeld       = true;
        lastTouchEdgeMs = now;
        jc_lastTapX = tx; jc_lastTapY = ty;
        if (ty <= BOT_Y) {
            Serial.printf("[jc3248w535] TAP TOP (%u,%u) -> next cyclic\n", tx, ty);
            switchToNextScreen();
        } else if (jc_currentScreen == SCR_SLIDESHOW) {
            // v0.9.8: bottom tap advances image instead of toggling pool/fees.
            Serial.printf("[jc3248w535] TAP BOT (%u,%u) -> next image\n", tx, ty);
            jc_slideshowAdvance();
        } else {
            Serial.printf("[jc3248w535] TAP BOT (%u,%u) -> toggle lower\n", tx, ty);
            toggleBottomScreen();
            cached_lower = -1;
        }
    } else if (!pressed) {
        touchHeld = false;
    }
}

#ifdef JC_TOUCH_DEBUG
static void jc_drawTouchDiag()
{
    if (!jc_panel) return;
    char buf[64];
    jc_panel->fillRect(0, JC_H - 12, 240, 12, BLACK);
    snprintf(buf, sizeof(buf), "P:%lu H:%lu r=%u,%u t=%u,%u",
             (unsigned long)jc_touchPolls, (unsigned long)jc_touchHits,
             jc_lastRawX, jc_lastRawY, jc_lastTapX, jc_lastTapY);
    jc_panel->setTextColor(YELLOW);
    jc_panel->setTextSize(1);
    jc_panel->setCursor(2, JC_H - 10);
    jc_panel->print(buf);
    jc_panel->fillRect(JC_W - 6, JC_H - 6, 4, 4, jc_heartbeatOn ? GREEN : DARKGREY);
    jc_panel->fillRect(JC_W - 14, JC_H - 14, 6, 6, touchHeld ? RED : RGB565(60,60,60));
}
#endif

// ============================================================================
//  High-water-mark size tracker (v0.9.17) — prevents font remnants
// ============================================================================
// Records max width/height per call-site (x,y); erase rect never shrinks.
// Safe on screen change (bg_cache fully regenerated).
#define JC_MAX_SIZE_SLOTS 32
struct JcSizeSlot { int x, y, max_w, max_h; };
static JcSizeSlot jc_sslots[JC_MAX_SIZE_SLOTS];
static int jc_sslot_count = 0;

static void jc_trackSize(int x, int y, int w, int h, int *out_w, int *out_h) {
    for (int i = 0; i < jc_sslot_count; i++) {
        if (jc_sslots[i].x == x && jc_sslots[i].y == y) {
            if (w > jc_sslots[i].max_w) jc_sslots[i].max_w = w;
            if (h > jc_sslots[i].max_h) jc_sslots[i].max_h = h;
            *out_w = jc_sslots[i].max_w;
            *out_h = jc_sslots[i].max_h;
            return;
        }
    }
    if (jc_sslot_count < JC_MAX_SIZE_SLOTS) {
        jc_sslots[jc_sslot_count] = {x, y, w, h};
        jc_sslot_count++;
    }
    *out_w = w;
    *out_h = h;
}

// ============================================================================
//  Drawing helpers — "dyn" variants restore bg from cache first
// ============================================================================
static void jc_textAt(int x, int y, uint16_t color, uint8_t size, const char *fmt, ...) {
    char buf[96]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    gfx->setTextColor(color); gfx->setTextSize(size); gfx->setCursor(x, y); gfx->print(buf);
}

// Erase via bg-cache restore, paint built-in font text, push rect.
// v0.9.17: high-water-mark w+h prevents remnant pixels on text shrink.
static void jc_dynText(int x, int y, int w, int h, uint16_t color, uint8_t size,
                       const char *fmt, ...) {
    char buf[96]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    gfx->setTextSize(size);
    int16_t tbx, tby; uint16_t tbw = 0, tbh;
    gfx->getTextBounds(buf, x, y, &tbx, &tby, &tbw, &tbh);
    int ew = (tbw > (uint16_t)w) ? (int)tbw : w;
    int eh = (tbh > (uint16_t)h) ? (int)tbh : h;
    // v0.9.17b: 20% descent padding for glyph overhang.
    eh = (eh * 6 + 4) / 5;   // ceil(eh * 1.20)
    jc_trackSize(x, y, ew, eh, &ew, &eh);
    jc_restoreRect(x, y, ew, eh);
    gfx->setTextColor(color); gfx->setCursor(x, y); gfx->print(buf);
    jc_flushOrPushRect(x, y, ew, eh);
}

// Erase via bg-cache restore, paint OFR TTF text, push rect.
// v0.9.17: high-water-mark w+h prevents remnant pixels on text shrink.
static void jc_dynOfr(JcFont face, int x, int y, int w, int h, uint16_t color,
                      uint16_t size_px, const char *fmt, ...) {
    if (!render_loaded) return;
    char buf[64]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    jc_loadFont(face);
    render.setFontSize(size_px);
    uint16_t actual_w = render.getTextWidth(buf);
    int ew = (actual_w > (uint16_t)w) ? (int)actual_w : w;
    // v0.9.17b: 20% descent padding — TTF glyphs extend below em-square.
    int eh = ((int)size_px > h) ? (int)size_px : h;
    eh = (eh * 6 + 4) / 5;   // ceil(eh * 1.20)
    jc_trackSize(x, y, ew, eh, &ew, &eh);
    jc_restoreRect(x, y, ew, eh);
    render.setFontColor(color);
    render.setCursor(x, y);
    render.printf("%s", buf);
    jc_flushOrPushRect(x, y, ew, eh);
}

// OFR draw without erase (for one-shot static labels)
static void jc_ofrText(JcFont face, int x, int y, uint16_t color, uint16_t size_px,
                       const char *fmt, ...) {
    if (!render_loaded) return;
    char buf[64]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    jc_loadFont(face);
    render.setFontSize(size_px);
    render.setFontColor(color);
    render.setCursor(x, y);
    render.printf("%s", buf);
}

// v0.9.17d: right-aligned variant — keys the size-tracker slot by a fixed
// anchor (slot_key, y) instead of the dynamic (x, y).  The erase rect always
// covers [slot_key .. JC_W] x [y .. y+eh], so no matter how text width changes,
// the old pixels are always cleaned up.
static void jc_dynOfrSlot(JcFont face, int slot_key, int y, uint16_t color,
                           uint16_t size_px, const char *fmt, ...) {
    if (!render_loaded) return;
    char buf[64]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    jc_loadFont(face);
    render.setFontSize(size_px);
    uint16_t actual_w = render.getTextWidth(buf);
    int x = JC_W - (int)actual_w - 32;  // 32px right margin
    if (x < 4) x = 4;
    int eh = (int)size_px;
    eh = (eh * 6 + 4) / 5;   // 20% descent padding
    int ew = JC_W - slot_key;  // erase from anchor to right edge
    jc_trackSize(slot_key, y, ew, eh, &ew, &eh);
    jc_restoreRect(slot_key, y, ew, eh);
    render.setFontColor(color);
    render.setCursor(x, y);
    render.printf("%s", buf);
    jc_flushOrPushRect(slot_key, y, ew, eh);
}

// ============================================================================
//  Background renderer — populates bg_cache then blits to canvas
// ============================================================================
static const unsigned short *jc_topArtFor(int cyc) {
    switch (cyc) {
        case SCR_MINER:  return MinerScreen;
        case SCR_CLOCK:  return minerClockScreen;
        case SCR_GLOBAL: return globalHashScreen;
        case SCR_PRICE:  return priceScreen;
        default:         return MinerScreen;
    }
}
static const unsigned short *jc_bottomArtFor(int lo) {
    switch (lo) {
        case LOWER_POOL: return bottonPoolScreen;
        case LOWER_FEES: return bottomMemPoolFees;
        default:         return bottonPoolScreen;
    }
}

// SD status pixel: GREEN=config present, CYAN=blank card, YELLOW=no card, RED=init failed.
// 4x4 px in bottom-left corner, drawn into bg_cache (survives until screen change).

#ifdef SDMMC_1BIT_FIX
#include "drivers/storage/SDCard.h"
#include "SD_MMC.h"   // v0.9.5: needed for SD_MMC.exists() cheap probe
extern SDCard SDCrd;
extern TSettings Settings;

// v0.9.4: ROOT CAUSE of false "no card" readings was wManager.cpp calling
// SDCrd.terminate() after config check, which tore down the SD bus.
// Fixed by patching wManager.cpp to skip terminate() under #ifdef JC3248W535.
// Polls at most once per JC_SD_POLL_MS from the per-tick path.
#define JC_SD_POLL_MS 10000
static uint32_t  sd_last_poll_ms     = 0;
static uint16_t  sd_cached_color     = YELLOW;
static bool      sd_have_polled_once = false;

static const char* jc_sdColorName(uint16_t c) {
    return c == GREEN  ? "GREEN"  :
           c == CYAN   ? "CYAN"   :
           c == YELLOW ? "YELLOW" : "OTHER";
}

static void jc_pollSdStatus()
{
#ifdef JC_NO_SD_PIXEL
    return;
#endif
    uint32_t now = millis();
    if (sd_have_polled_once && (now - sd_last_poll_ms) <= JC_SD_POLL_MS) return;
    sd_last_poll_ms = now;

    // v0.9.5: SD_MMC.exists() + cardType() avoids JSON re-parse and WiFi password leakage.
    uint16_t newColor;
    if (SD_MMC.cardType() == CARD_NONE) {
        newColor = YELLOW;        // not mounted (or driver torn down)
    } else if (SD_MMC.exists("/config.json")) {
        newColor = GREEN;         // card has a config file
    } else {
        newColor = CYAN;          // card mounted, no config
    }

    bool firstPoll = !sd_have_polled_once;
    bool changed   = (newColor != sd_cached_color);
    sd_cached_color = newColor;
    sd_have_polled_once = true;

    // Log only on first poll or state change.
    if (firstPoll || changed) {
        Serial.printf("[jc3248w535] SD pixel %s @%lus: color=%s\n",
            firstPoll ? "init" : "change",
            (unsigned long)(now / 1000),
            jc_sdColorName(newColor));
    }
}

static void jc_drawSdStatusPixel()
{
    if (!bg_cache) return;
    // Ensure at least one poll before first paint.
    jc_pollSdStatus();
    for (int dy = 0; dy < 4; dy++)
        for (int dx = 0; dx < 4; dx++)
            *bg_pix(2 + dx, JC_H - 6 + dy) = sd_cached_color;
}
#endif

static void jc_renderBackground(int cyc, int lo)
{
    if (!gfx || !bg_cache) return;
    Serial.printf("[jc3248w535] bg render: cycle=%d lower=%d\n", cyc, lo);

    // Ensure pre-rendered slabs exist; fall back to scaling path if PSRAM exhausted.
    bool top_ok = jc_ensureTopSlab(cyc);
    bool bot_ok = jc_ensureBotSlab(lo);

    // Compose bg_cache from slabs (fast memcpy) or fall back to NN scaling.
    if (top_ok) {
        jc_blitTopSlabToBgCache(cyc);
    } else {
        jc_scaleIntoBgCache(jc_topArtFor(cyc),
                             SRC_W, SRC_TOP_H, TOP_X, TOP_Y, TOP_W, TOP_H);
    }
    if (bot_ok) {
        jc_blitBotSlabToBgCache(lo);
    } else {
        jc_scaleIntoBgCache(jc_bottomArtFor(lo),
                             SRC_W, SRC_BOT_H, BOT_X, BOT_Y, BOT_W, BOT_H);
    }

    // SD status pixel (painted into bg_cache, survives text overlays).
#if defined(SDMMC_1BIT_FIX) && !defined(JC_NO_SD_PIXEL)
    jc_drawSdStatusPixel();
#endif

    // Blit full bg_cache to canvas (only big canvas write per screen-change).
    jc_blitBgCacheToCanvas();
}

static bool jc_ensureBackground(int cyc, int lo) {
    if (cached_cycle != cyc || cached_lower != lo) {
        jc_renderBackground(cyc, lo);
        cached_cycle = cyc;
        cached_lower = lo;
        return true;
    }
    return false;
}

// ============================================================================
//  Init / state hooks
// ============================================================================
void jc3248w535_Init(void)
{
    // HWCDC Serial.write() BLOCKS (100ms timeout) when no USB host drains TX buffer.
    // Timeout=0 makes write() drop data silently on headless boot.
#if defined(ARDUINO_USB_CDC_ON_BOOT) && defined(ARDUINO_USB_MODE)
    Serial.setTxTimeoutMs(0);
#endif

    // Non-blocking serial (flush BLOCKS forever on headless boot). 3 banners for late connect.
    for (int i = 0; i < 3; i++) {
#ifdef JC_USE_PARTIAL_FLUSH
        Serial.println(F("[jc3248w535] init v0.9.17d (BL gating; no-op LED; remnant fix) ##"));
#else
        Serial.println(F("[jc3248w535] init v0.9.17d (BL gating; no-op LED; remnant fix) ##"));
#endif
    }
    delay(50);   // brief gap to let USB-CDC enumerate if a host is plugged in
    Serial.printf("[jc3248w535] free heap=%u psram=%u\n",
                  ESP.getFreeHeap(), ESP.getFreePsram());

    jc_bus   = new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
    jc_panel = new Arduino_AXS15231B(jc_bus, GFX_NOT_DEFINED, 0, false, 320, 480);
    gfx      = new Arduino_Canvas(320, 480, jc_panel, 0, 0, 0);
    if (!gfx || !gfx->begin()) { Serial.println(F("[jc3248w535] gfx->begin() FAILED")); return; }

    // v0.9.16a: reclaim BL GPIO after gfx->begin() (library may reconfigure pins).
    pinMode(JC_BL_PIN, OUTPUT);
    digitalWrite(JC_BL_PIN, HIGH);

    gfx->setRotation(1); gfx->fillScreen(BLACK); JC_FLUSH();

    if (!jc_allocBgCache()) {
        Serial.println(F("[jc3248w535] !! bg_cache alloc failed; transparency will fail"));
    }

    render.setDrawPixel(jc_ofr_drawPixel);
    render.setDrawFastHLine(jc_ofr_drawHLine);
    render.setStartWrite(jc_ofr_startWrite);
    render.setEndWrite(jc_ofr_endWrite);
    render.setLineSpaceRatio(0.9);
    jc_loadFont(JcFont::Digital);

    Wire.begin(TOUCH_SDA, TOUCH_SCL); Wire.setClock(TOUCH_I2C_HZ);
    // TOUCH_INT/RST not driven — shared with SDMMC (CLK=12, CMD=11). Chip self-resets.
    // If your board variant needs external RST, restore:
    //   pinMode(TOUCH_RST, OUTPUT); digitalWrite(TOUCH_RST, LOW); delay(50);
    //   digitalWrite(TOUCH_RST, HIGH); delay(150);
    // ...and undef SDMMC_1BIT_FIX.
    delay(200);  // wait for chip POR
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Serial.printf("[jc3248w535] touch %s @ 0x%02X\n",
                  (Wire.endTransmission() == 0) ? "OK" : "MISSING", TOUCH_I2C_ADDR);
    Serial.printf("[jc3248w535] layout top=%dx%d bot=%dx%d (fill width)\n",
                  TOP_W, TOP_H, BOT_W, BOT_H);

#ifdef SDMMC_1BIT_FIX
    // v0.9.3: internal pull-ups on SDMMC pins — this board lacks onboard 10k pull-ups,
    // so bus floats and card releases session after ~1s. ESP32 internal (~45 kΩ) is
    // weaker but sufficient for short PCB traces. Set before SD_MMC.begin() (it won't
    // touch the pull-up enable bit, only GPIO matrix routing).
    pinMode(SDMMC_CLK, INPUT_PULLUP);
    pinMode(SDMMC_CMD, INPUT_PULLUP);
    pinMode(SDMMC_D0,  INPUT_PULLUP);
    Serial.printf("[jc3248w535] SDMMC pull-ups enabled on GPIO %d/%d/%d\n",
                  SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
#endif
}

void jc3248w535_AlternateScreenState(void)
{
    jc_backlight_enabled = !jc_backlight_enabled;
    digitalWrite(JC_BL_PIN, jc_backlight_enabled ? HIGH : LOW);
}

void jc3248w535_AlternateRotation(void) {
    int r = (gfx->getRotation() == 1) ? 3 : 1;
    gfx->setRotation(r); gfx->fillScreen(BLACK);
    JC_FLUSH();
    cached_cycle = -1; cached_lower = -1;
    for (int i = 0; i < SCR_COUNT; i++) last_snapshot[i] = "";
}

// ============================================================================
//  Loading & Setup — no bg cache used (one-shot screens)
// ============================================================================
void jc3248w535_LoadingScreen(void)
{
    if (!gfx) return;
    gfx->fillScreen(BLACK);
    // Scale 320x170 init art to 480x255 directly (one-shot, no cache needed).
    for (int dy = 0; dy < 255; dy++) {
        int sy_s = (dy * 170) / 255;
        const unsigned short *row = initScreen + sy_s * 320;
        for (int dx = 0; dx < 480; dx++) {
            int sx_s = (dx * 320) / 480;
            gfx->writePixel(dx, dy, pgm_read_word(&row[sx_s]));
        }
    }
    jc_textAt(sx(24), sy_top(147), BLACK, 1, "v%s", CURRENT_VERSION);
    jc_textAt(8, 264, WHITE, 2, "Initializing NerdMiner...");
    jc_textAt(8, 288, RGB565(0,180,220), 2, "Display driver by @cosmicpsyop");
    jc_textAt(8, 312, DARKGREY, 1, "Guition JC3248W535 / 8MB PSRAM / 16MB Flash");

    JC_FLUSH();
    cached_cycle = -1; cached_lower = -1;
}

void jc3248w535_SetupScreen(void)
{
    if (!gfx) return;
    gfx->fillScreen(BLACK);
    for (int dy = 0; dy < 255; dy++) {
        int sy_s = (dy * 170) / 255;
        const unsigned short *row = setupModeScreen + sy_s * 320;
        for (int dx = 0; dx < 480; dx++) {
            int sx_s = (dx * 320) / 480;
            gfx->writePixel(dx, dy, pgm_read_word(&row[sx_s]));
        }
    }
    jc_textAt(8,  264, YELLOW, 2, "WiFi config required");
    jc_textAt(8,  286, CYAN,   2, "Join 'NerdMinerAP' WiFi");
    jc_textAt(8,  306, WHITE, 1, "Then browse to http://192.168.4.1");
    JC_FLUSH();
    cached_cycle = -1; cached_lower = -1;
}

// ============================================================================
//  Bottom panel text overlays
// ============================================================================
//         lowered 2 screen-px, font size up to 24px ("a little larger").
//         Erase rect height bumped to 28 to cover taller glyphs.
static void jc_drawLowerPool(unsigned long /*mElapsed*/)
{
    pool_data p = getPoolData();
    jc_dynOfr(JcFont::Digital, sx(5),   sy_bot(34) + 2, sx(80) - sx(5),    28,
                      BLACK, 24, "%s", p.bestDifficulty.c_str());
    jc_dynOfr(JcFont::Digital, sx(146), sy_bot(34) + 6, 60,                 32,
                      BLACK, 28, "%d", p.workersCount);
    jc_dynOfr(JcFont::Digital, sx(216), sy_bot(34) + 2, sx(315) - sx(216),  28,
                      BLACK, 24, "%s", p.workersHash.c_str());
}

static void jc_drawLowerFees(unsigned long mElapsed)
{
    coin_data c = getCoinData(mElapsed);
    jc_dynOfr(JcFont::Digital, sx(30),  sy_bot(32) + 2, 60, 28, BLACK, 24, "%s", c.fastestFee.c_str());
    jc_dynOfr(JcFont::Digital, sx(140), sy_bot(38) + 2, 80, 28, BLACK, 24, "%s", c.economyFee.c_str());
    jc_dynOfr(JcFont::Digital, sx(250), sy_bot(32) + 2, 60, 28, RED,   24, "%s", c.minimumFee.c_str());
}

// v0.9.17d: slideshow entry tracking — declared unconditionally so all
// cyclic screens can clear it. Only meaningfully set inside #ifdef JC_HAS_SLIDESHOW.
static bool jc_ss_active = false;

// ============================================================================
//  Cyclic screens
// ============================================================================
static void jc3248w535_MinerScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_currentScreen = SCR_MINER;
    jc_ss_active = false;  // v0.9.17d: clear slideshow entry flag
    jc_ensureBackground(SCR_MINER, lowerScreen);
    mining_data data = getMiningData(mElapsed);

    // Hashrate (big, lower-left area of art)
    jc_dynOfr(JcFont::Digital, sx(50) - 51, sy_top(110), sx(180)-(sx(50)-51), 44,
                      BLACK, 50, "%s", data.currentHashRate.c_str());

    // Stats column on the right side of art (templates / bestDiff / shares)
    jc_dynOfr(JcFont::Digital, sx(186) + 4, sy_top(20), 60, 20, WHITE, 20,
                      "%s", data.templates.c_str());
    jc_dynOfr(JcFont::Digital, sx(186), sy_top(48), 60, 20, WHITE, 20,
                      "%s", data.bestDiff.c_str());
    jc_dynOfr(JcFont::Digital, sx(186), sy_top(76), 60, 20, WHITE, 20,
                      "%s", data.completedShares.c_str());

    // Valid blocks (big number in its own slot)
    jc_dynOfr(JcFont::Digital, sx(285), sy_top(54), 40, 32, WHITE, 32,
                      "%s", data.valids.c_str());

    // Time mining ("uptime"), right side near bottom of top art.
    jc_dynOfr(JcFont::Digital, sx(180) + 31, sy_top(106), sx(315)-(sx(180)+31), 20,
                      WHITE, 16, "%s", data.timeMining.c_str());

    // Total MHashes ("million hashes") — DigitalNumbers via OFR @ 22px.
    jc_dynOfr(JcFont::Digital, sx(220) - 26, sy_top(142), sx(295)-sx(220), 26,
                      BLACK, 22, "%s", data.totalMHashes.c_str());

    // Temp + time at top of art (built-in font, size 2)
    jc_dynText(sx(225), sy_top(4), 40, 14, BLACK, 2, "%s", data.temp.c_str());
    {
        // Degree symbol — position depends on temp string length.
        int chars = (int)strlen(data.temp.c_str());
        int dot_x = sx(225) + chars * 12 + 2;
        int dot_y = sy_top(4) + 2;
        gfx->drawCircle(dot_x, dot_y, 2, BLACK);
    }
    jc_dynText(sx(249), sy_top(4), sx(315)-sx(249), 14, BLACK, 2,
                       "%s",  data.currentTime.c_str());

    // Bottom panel
    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    pool_data pds = getPoolData();
    coin_data cds = getCoinData(mElapsed);
    // Quantized snapshot: hashrate=q10 hysteresis, time=HH:MM, temp=whole-degree hysteresis.
    // Other fields (shares, valids, etc.) change slowly — no quantization needed.
    String snap = jc_qHashrateH(data.currentHashRate, &jc_hr_state_miner) + "|" + data.templates + "|" + data.bestDiff
                + "|" + data.completedShares + "|" + data.valids + "|" + jc_qTimeMin(data.timeMining)
                + "|" + data.totalMHashes + "|" + jc_qTemp(data.temp) + "|" + jc_qTimeMin(data.currentTime)
                + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + jc_qHashrateH(pds.workersHash, &jc_hr_state_miner_pool))
                    : (cds.fastestFee + "/" + cds.economyFee + "/" + cds.minimumFee));
    if (jc_shouldFlush(SCR_MINER, snap)) jc_endOfFrameFlush(jc_last_bg_changed);

    Serial.printf(">>> Miner: %s share(s)  hash %s KH/s\n",
                  data.completedShares.c_str(), data.currentHashRate.c_str());
    jc_pollTouch();
}

static void jc3248w535_ClockScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_currentScreen = SCR_CLOCK;
    jc_ss_active = false;
    jc_ensureBackground(SCR_CLOCK, lowerScreen);
    clock_data data = getClockData(mElapsed);

    // Big clock — right-aligned, NotoBold, WHITE. Uses fixed slot_key so
    // the erase rect doesn't fragment as time string width changes.
    // jc_dynOfrSlot(JcFont::NotoBold, JC_W - 200, sy_top(40), WHITE, 78,
    jc_dynOfrSlot(JcFont::NotoBold, 4, sy_top(40), WHITE, 78,
                  "%s", data.currentTime.c_str());

    // Hashrate lower-left
    jc_dynOfr(JcFont::Digital, sx(19), sy_top(128), sx(120)-sx(19), 38,
              BLACK, 34, "%s", data.currentHashRate.c_str());

    // Block height lower-right — DigitalNumbers @ 26px
    jc_dynOfr(JcFont::Digital, sx(180) - 9, sy_top(140), sx(290)-(sx(180)-9), 26,
              BLACK, 26, "%s", data.blockHeight.c_str());

    // BTC price top of art
    jc_dynText(sx(195) + 15, sy_top(7), sx(315)-(sx(195)+15), 14, BLACK, 2,
               "%s", data.btcPrice.c_str());

    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    pool_data pds = getPoolData();
    coin_data cds = getCoinData(mElapsed);
    // Quantized — see Miner snapshot above.
    String snap = jc_qTimeMin(data.currentTime) + "|" + data.btcPrice + "|" + jc_qHashrateH(data.currentHashRate, &jc_hr_state_clock)
                + "|" + data.blockHeight + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + jc_qHashrateH(pds.workersHash, &jc_hr_state_clock_pool))
                    : (cds.fastestFee + "/" + cds.economyFee + "/" + cds.minimumFee));
    if (jc_shouldFlush(SCR_CLOCK, snap)) jc_endOfFrameFlush(jc_last_bg_changed);

    Serial.printf(">>> Clock %s  BTC %s\n", data.currentTime.c_str(), data.btcPrice.c_str());
    jc_pollTouch();
}

static void jc3248w535_GlobalHashScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_currentScreen = SCR_GLOBAL;
    jc_ss_active = false;
    jc_ensureBackground(SCR_GLOBAL, lowerScreen);
    coin_data data = getCoinData(mElapsed);
    clock_data ctm = getClockData(mElapsed);

    jc_dynText(sx(195) + 15, sy_top(7), 100, 14, BLACK, 2, "%s",
               data.btcPrice.c_str());
    // Both at sy_top(7).
    jc_dynText(sx(265), sy_top(7), sx(315)-sx(265), 14, BLACK, 2,
               "%s", ctm.currentTime.c_str());

    jc_dynText(sx(240), sy_top(53), sx(315)-sx(240), 18, WHITE, 2,
               "%s", data.halfHourFee.c_str());
    jc_dynText(sx(240), sy_top(88), sx(315)-sx(240), 18, WHITE, 2,
               "%s", data.netwrokDifficulty.c_str());

    jc_dynOfr(JcFont::Digital, sx(190) + 75, sy_top(143), sx(290)-(sx(190)+75) + 60, 24,
              BLACK, 24, "%s", data.globalHashRate.c_str());
    jc_dynOfr(JcFont::Digital, sx(20), sy_top(100), sx(150)-sx(20), 36,
              WHITE, 36, "%s", data.blockHeight.c_str());

    // Halving progress bar — restore the slot first, then draw fill + label
    int bx = sx(2), by = sy_top(149), bw = sx(140)-sx(2), bh = sy_top(168)-by;
    jc_restoreRect(bx, by, bw, bh);
    int fillW = (int)(bw * data.progressPercent / 100.0f);
    if (fillW > 0) gfx->fillRect(bx, by, fillW, bh, RGB565(220, 220, 220));
    gfx->setTextColor(BLACK); gfx->setTextSize(1);
    int16_t tbx, tby; uint16_t tbw, tbh;
    char lbl[40]; snprintf(lbl, sizeof(lbl), "%s blocks", data.remainingBlocks.c_str());
    gfx->getTextBounds(lbl, 0, 0, &tbx, &tby, &tbw, &tbh);
    gfx->setCursor(bx + (bw - (int)tbw)/2, by + 4);
    gfx->print(lbl);

    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    pool_data pds = getPoolData();
    // Quantized time + hashrates. globalHashRate drifts slowly (no quantization).
    String snap = data.btcPrice + "|" + jc_qTimeMin(ctm.currentTime) + "|" + data.halfHourFee
                + "|" + data.netwrokDifficulty + "|" + data.globalHashRate
                + "|" + data.blockHeight + "|" + String((int)(data.progressPercent * 10))
                + "|" + data.remainingBlocks + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + jc_qHashrateH(pds.workersHash, &jc_hr_state_global_pool))
                    : (data.fastestFee + "/" + data.economyFee + "/" + data.minimumFee));
    if (jc_shouldFlush(SCR_GLOBAL, snap)) jc_endOfFrameFlush(jc_last_bg_changed);

    Serial.printf(">>> Global BTC %s  net %s\n",
                  data.btcPrice.c_str(), data.globalHashRate.c_str());
    jc_pollTouch();
}

static void jc3248w535_PriceScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_currentScreen = SCR_PRICE;
    jc_ss_active = false;
    jc_ensureBackground(SCR_PRICE, lowerScreen);
    clock_data data = getClockData(mElapsed);

    // Time — built-in size 2 at sy_top(7). X preserved per user feedback.
    jc_dynText(sx(220), sy_top(7), sx(315) - sx(220), 14, BLACK, 2,
               "%s", data.currentTime.c_str());

    // BIG BTC price — right-aligned, NotoBold. Fixed slot_key for clean erase.
    //jc_dynOfrSlot(JcFont::NotoBold, JC_W - 250, sy_top(46), WHITE, 64,
    jc_dynOfrSlot(JcFont::NotoBold, 4, sy_top(46), WHITE, 64,
                  "%s", data.btcPrice.c_str());

    //         BLACK (was WHITE), font size 30 -> 34
    jc_dynOfr(JcFont::Digital, sx(19), sy_top(128), sx(120)-sx(19), 38,
              BLACK, 34, "%s", data.currentHashRate.c_str());
    jc_dynOfr(JcFont::Digital, sx(180), sy_top(137), sx(290)-sx(180), 24,
              WHITE, 24, "%s", data.blockHeight.c_str());

    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    pool_data pds = getPoolData();
    coin_data cds = getCoinData(mElapsed);
    // Quantized — see Miner snapshot above. (This screen had worst q250<->q260 oscillation.)
    String snap = data.btcPrice + "|" + jc_qTimeMin(data.currentTime) + "|" + jc_qHashrateH(data.currentHashRate, &jc_hr_state_price)
                + "|" + data.blockHeight + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + jc_qHashrateH(pds.workersHash, &jc_hr_state_price_pool))
                    : (cds.fastestFee + "/" + cds.economyFee + "/" + cds.minimumFee));
    if (jc_shouldFlush(SCR_PRICE, snap)) jc_endOfFrameFlush(jc_last_bg_changed);

    Serial.printf(">>> Price BTC %s  hash %s KH/s\n",
                  data.btcPrice.c_str(), data.currentHashRate.c_str());
    jc_pollTouch();
}

// ============================================================================
//  JPEG slideshow (5th cyclic screen, v0.9.8)
// ============================================================================
// Reads /pic/*.jpg from SD, decodes via TJpgDec, renders full canvas with a
// bottom stats overlay (time / BTC / block height). Auto-advances per period;
// bottom tap advances image, top tap goes to next cyclic. Graceful degradation
// without SD or /pic dir. No extra framebuffer — TJpgDec decodes MCU-by-MCU.
#if defined(SDMMC_1BIT_FIX) && !defined(JC_NO_SLIDESHOW)
#define JC_HAS_SLIDESHOW 1
#endif

#ifdef JC_HAS_SLIDESHOW
#include <TJpg_Decoder.h>
#include <vector>

#define JC_SLIDESHOW_PERIOD_MS  16000UL  // v0.9.9: 16s (was 8s)
#define JC_SLIDESHOW_DIR        "/pic"
#define JC_SLIDESHOW_MAX_FILES  64
#define JC_SLIDESHOW_OVERLAY_H  28          // v0.9.17b: compact NotoBold overlay

static std::vector<String> jc_ss_files;
static int                 jc_ss_idx        = 0;
static int                 jc_ss_loadedIdx  = -1;    // which image is currently decoded into the canvas
static uint32_t            jc_ss_lastAdv_ms = 0;
static bool                jc_ss_dirScanned = false;
static bool                jc_ss_failedScan = false;

// TJpgDec MCU render callback. RGB565 native-endian. v0.9.9: image fills full
// canvas (overlay draws on top) — previously clipped, causing letterbox bars.
static bool jc_ss_renderCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    if (!gfx) return false;
    gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
    return true;                                         // continue decoding
}

static void jc_ss_scanDir()
{
    jc_ss_files.clear();
    jc_ss_failedScan = false;
    if (SD_MMC.cardType() == CARD_NONE) {
        jc_ss_failedScan = true;
        Serial.println(F("[jc3248w535] slideshow: SD not mounted"));
        return;
    }
    File root = SD_MMC.open(JC_SLIDESHOW_DIR);
    if (!root || !root.isDirectory()) {
        jc_ss_failedScan = true;
        Serial.printf("[jc3248w535] slideshow: %s missing or not a dir\n",
                      JC_SLIDESHOW_DIR);
        return;
    }
    File f = root.openNextFile();
    while (f && (int)jc_ss_files.size() < JC_SLIDESHOW_MAX_FILES) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            String lower = name; lower.toLowerCase();
            if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) {
                // f.name() returns basename; reconstruct full path (varies by core).
                String path;
                if (name.startsWith("/")) path = name;
                else                      path = String(JC_SLIDESHOW_DIR) + "/" + name;
                jc_ss_files.push_back(path);
            }
        }
        f.close();
        f = root.openNextFile();
    }
    root.close();
    jc_ss_dirScanned = true;
    Serial.printf("[jc3248w535] slideshow: scanned %s, found %u jpg(s)\n",
                  JC_SLIDESHOW_DIR, (unsigned)jc_ss_files.size());
    for (size_t i = 0; i < jc_ss_files.size(); i++) {
        Serial.printf("  [%u] %s\n", (unsigned)i, jc_ss_files[i].c_str());
    }
}

static void jc_ss_drawNoImagesMessage()
{
    if (!gfx) return;
    gfx->fillScreen(BLACK);
    jc_textAt(40, JC_H / 2 - 24, WHITE, 2, "Slideshow");
    if (SD_MMC.cardType() == CARD_NONE) {
        jc_textAt(40, JC_H / 2,      YELLOW, 2, "No SD card");
        jc_textAt(40, JC_H / 2 + 24, DARKGREY, 1, "Insert a FAT32 SD card and reboot.");
    } else {
        jc_textAt(40, JC_H / 2,      YELLOW, 2, "No images");
        jc_textAt(40, JC_H / 2 + 24, DARKGREY, 1, "Add .jpg files to /pic on the SD card.");
        jc_textAt(40, JC_H / 2 + 40, DARKGREY, 1, "Max 480x320 recommended.");
    }
}

static void jc_ss_loadCurrent()
{
    if (!gfx) return;
    if (jc_ss_files.empty()) {
        jc_ss_drawNoImagesMessage();
        JC_FLUSH();  // gate
        jc_ss_loadedIdx = -2;     // -2 = "no-images placeholder drawn"
        return;
    }
    if (jc_ss_idx < 0) jc_ss_idx = (int)jc_ss_files.size() - 1;
    if (jc_ss_idx >= (int)jc_ss_files.size()) jc_ss_idx = 0;
    const char *path = jc_ss_files[jc_ss_idx].c_str();
    Serial.printf("[jc3248w535] slideshow: loading [%d/%u] %s\n",
                  jc_ss_idx, (unsigned)jc_ss_files.size(), path);

    TJpgDec.setJpgScale(1);          // start with no downscale
    TJpgDec.setSwapBytes(false);     // RGB565 native order
    TJpgDec.setCallback(jc_ss_renderCb);

    // Clear full canvas (image fills 480x320, overlay draws on top).
    gfx->fillScreen(BLACK);

    // Get JPEG dims to center. Use FS-based API (SD_MMC, not SD-SPI).
    uint16_t imgW = 0, imgH = 0;
    TJpgDec.getFsJpgSize(&imgW, &imgH, path, SD_MMC);
    int origin_x = 0, origin_y = 0;
    if (imgW > 0 && imgH > 0) {
        // Scale-to-fit against full canvas (previously left letterbox strip).
        int scale = 1;
        while ((imgW / scale) > JC_W || (imgH / scale) > JC_H) {
            scale *= 2;
            if (scale > 8) break;
        }
        if (scale > 1) TJpgDec.setJpgScale(scale);
        int dispW = imgW / scale;
        int dispH = imgH / scale;
        origin_x = (JC_W - dispW) / 2;
        origin_y = (JC_H - dispH) / 2;
        if (origin_x < 0) origin_x = 0;
        if (origin_y < 0) origin_y = 0;
        Serial.printf("[jc3248w535] slideshow: %ux%u -> scale=%d -> %ux%u @(%d,%d)\n",
                      imgW, imgH, scale, dispW, dispH, origin_x, origin_y);
    }

    JRESULT jr = TJpgDec.drawFsJpg(origin_x, origin_y, path, SD_MMC);
    if (jr != JDR_OK) {
        Serial.printf("[jc3248w535] slideshow: TJpgDec.drawFsJpg failed: %d\n", jr);
        gfx->fillScreen(BLACK);
        static const char * const jc_jpg_err[] = {
           "OK",                     // 0 JDR_OK
           "File read error",         // 1 JDR_INP
           "Insufficient work memory",// 2 JDR_MEM1
           "Insufficient stream buf", // 3 JDR_MEM2
           "Invalid JPEG structure",  // 4 JDR_PAR
           "Unsupported format",      // 5 JDR_FMT1
           "Unsupported subsampling", // 6 JDR_FMT2
           "Unsupported color space", // 7 JDR_FMT3
        };
        int ei = (jr >= 0 && jr <= 7) ? jr : 4;  // unknown -> PAR
        jc_textAt(20, JC_H / 2 - 24, RED, 2, "JPEG error");
        jc_textAt(20, JC_H / 2,      WHITE, 1, jc_jpg_err[ei]);

        jc_textAt(20, JC_H / 2 + 16, DARKGREY, 1, path);
    }

    // Push canvas once; overlay redraws each tick on top.
    JC_FLUSH();
    jc_ss_loadedIdx = jc_ss_idx;
}

static void jc_slideshowAdvance()
{
    if (jc_ss_files.empty()) return;
    jc_ss_idx = (jc_ss_idx + 1) % (int)jc_ss_files.size();
    jc_ss_loadedIdx = -1;            // force reload on next screen tick
    jc_ss_lastAdv_ms = millis();
}

// v0.9.17d: true when slideshow has usable images. Evaluated once on first entry.
static bool jc_ss_available = false;

static void jc3248w535_SlideshowScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_currentScreen = SCR_SLIDESHOW;

    // Lazy-scan SD on first entry.
    if (!jc_ss_dirScanned || jc_ss_failedScan) {
        jc_ss_scanDir();
    }

    // v0.9.17b: skip carousel entry if no images found.
    if (!jc_ss_available) {
        if (jc_ss_dirScanned && !jc_ss_failedScan && jc_ss_files.size() > 0) {
            jc_ss_available = true;
        } else {
            Serial.println(F("[jc3248w535] slideshow: no images — skipping in carousel"));
            switchToNextScreen();
            return;
        }
    }

    // v0.9.17d: detect fresh slideshow entry — invalidate bg cache
    // and force image reload each time we enter from another screen.
    if (!jc_ss_active) {
        jc_ss_active = true;
        cached_cycle = -1; cached_lower = -1;
        for (int i = 0; i < SCR_COUNT; i++) last_snapshot[i] = "";
        jc_ss_loadedIdx = -1;  // force image reload on re-entry
    }

    uint32_t now = millis();
    // Auto-advance every JC_SLIDESHOW_PERIOD_MS.
    if (jc_ss_loadedIdx >= 0 && (now - jc_ss_lastAdv_ms) >= JC_SLIDESHOW_PERIOD_MS) {
        jc_slideshowAdvance();
    }
    // (Re)load the current image if it's not already drawn.
    if (jc_ss_loadedIdx != jc_ss_idx) {
        jc_ss_loadCurrent();
        jc_ss_lastAdv_ms = now;
        // New image wiped canvas — force overlay redraw.
        last_snapshot[SCR_SLIDESHOW] = "";
    }

    // v0.9.17d: build snapshot first — only redraw overlay on data change.
    coin_data cd  = getCoinData(mElapsed);
    clock_data ck  = getClockData(mElapsed);

    // Extract HH:MM from ck.currentTime ("HH:MM:SS" -> "HH:MM").
    String hhmm = ck.currentTime;
    if (hhmm.length() >= 5) hhmm = hhmm.substring(0, 5);

    String snap = hhmm + "|" + cd.btcPrice + "|" + ck.blockHeight;

    if (jc_shouldFlush(SCR_SLIDESHOW, snap)) {

        int oy = JC_H - JC_SLIDESHOW_OVERLAY_H;  // 320 - 28 = 292

        // White 1 px separator line
        gfx->drawFastHLine(0, oy, JC_W, WHITE);
        // Dark backdrop (not pure black — subtle blue tint for depth)
        gfx->fillRect(0, oy + 1, JC_W, JC_SLIDESHOW_OVERLAY_H - 1, RGB565(8, 8, 18));

        // Draw all three items with NotoBold @ 18 px on a single baseline.
        if (render_loaded) {
            jc_loadFont(JcFont::NotoBold);
            render.setFontSize(18);
            render.setFontColor(WHITE);

            char buf[48];
            int baseline = oy + 1 + (JC_SLIDESHOW_OVERLAY_H - 1 - 18) / 2;

            // Time (left)
            render.setCursor(16, baseline);
            render.printf("%s", hhmm.c_str());

            // BTC price (centered)
            snprintf(buf, sizeof(buf), "$%s", cd.btcPrice.c_str());
            uint16_t pw = render.getTextWidth(buf);
            render.setCursor((JC_W - (int)pw) / 2, baseline);
            render.printf("%s", buf);

            // Block height (right-aligned)
            snprintf(buf, sizeof(buf), "blk %s", ck.blockHeight.c_str());
            uint16_t bw = render.getTextWidth(buf);
            render.setCursor(JC_W - 16 - (int)bw, baseline);
            render.printf("%s", buf);
        } else {
            // Fallback to built-in font if OFR not loaded
            gfx->setTextColor(WHITE); gfx->setTextSize(2);
            char buf[48];
            int baseline = oy + 5;
            snprintf(buf, sizeof(buf), "%s", hhmm.c_str());
            gfx->setCursor(8, baseline); gfx->print(buf);
            snprintf(buf, sizeof(buf), "BTC $%s", cd.btcPrice.c_str());
            int px = JC_W - 8 - (int)strlen(buf) * 12;
            gfx->setCursor(px, baseline); gfx->print(buf);
            snprintf(buf, sizeof(buf), "blk %s", ck.blockHeight.c_str());
            int bx = (JC_W - (int)strlen(buf) * 12) / 2;
            gfx->setCursor(bx, baseline + 16); gfx->print(buf);
        }

        jc_endOfFrameFlush(true);
    }

    Serial.printf(">>> Slideshow [%d/%u] %s  BTC $%s  blk %s\n",
                  jc_ss_idx, (unsigned)jc_ss_files.size(),
                  hhmm.c_str(), cd.btcPrice.c_str(), ck.blockHeight.c_str());
    jc_pollTouch();
}
#else
// SD disabled: stubs for compile parity; shows "disabled" message.
static void jc_slideshowAdvance() {}
static void jc3248w535_SlideshowScreen(unsigned long /*mElapsed*/)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_currentScreen = SCR_SLIDESHOW;
    cached_cycle = -1; cached_lower = -1;
    gfx->fillScreen(BLACK);
    jc_textAt(40, JC_H / 2 - 8, YELLOW, 2, "Slideshow");
    jc_textAt(40, JC_H / 2 + 12, DARKGREY, 1, "SD card disabled in this build.");
    JC_FLUSH();
    jc_pollTouch();
}
#endif

// ============================================================================
//  Animate / LED
// ============================================================================
void jc3248w535_AnimateCurrentScreen(unsigned long /*frame*/)
{
    jc_animateCount++;
    jc_heartbeatOn = !jc_heartbeatOn;
    jc_pollTouch();
#ifdef JC_TOUCH_DEBUG
    jc_drawTouchDiag();
#endif
}

void jc3248w535_DoLedStuff(unsigned long /*frame*/)
{
    // LED_PIN == LCD_BL (GPIO 1) on this board — no-op to protect backlight.
}

// ============================================================================
//  Driver table
// ============================================================================
CyclicScreenFunction jc3248w535CyclicScreens[] = {
    jc3248w535_MinerScreen,
    jc3248w535_ClockScreen,
    jc3248w535_GlobalHashScreen,
    jc3248w535_PriceScreen,
#ifdef JC_HAS_SLIDESHOW
    jc3248w535_SlideshowScreen,
#endif
};

DisplayDriver jc3248w535DisplayDriver = {
    jc3248w535_Init,
    jc3248w535_AlternateScreenState,
    jc3248w535_AlternateRotation,
    jc3248w535_LoadingScreen,
    jc3248w535_SetupScreen,
    jc3248w535CyclicScreens,
    jc3248w535_AnimateCurrentScreen,
    jc3248w535_DoLedStuff,
    SCREENS_ARRAY_SIZE(jc3248w535CyclicScreens),
    0,
    JC_W,
    JC_H,
};

#endif // JC3248W535_DISPLAY
