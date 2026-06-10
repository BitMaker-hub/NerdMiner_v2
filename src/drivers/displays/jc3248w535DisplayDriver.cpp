// =============================================================================
//  Guition JC3248W535 display driver for NerdMiner v2  (v0.8.27)
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
//                changed OR every JC_MAX_FLUSH_GAP_MS (5s safety net so
//                layout edits become visible without value churn).
//    * Touch debug overlay: compile with -D JC_TOUCH_DEBUG to enable a
//                live counter strip at the bottom of the panel.
//
//  See git log for change history.
// =============================================================================

#include "displayDriver.h"

#ifdef JC3248W535_DISPLAY

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <esp_heap_caps.h>

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

#define SRC_W          320
#define SRC_TOP_H      170
#define SRC_BOT_H       70

// TOP: 1.5x both axes -> 480x255
// Top scaling becomes H=1.5x, V=1.294x (was 1.5x both).
// Bottom scaling becomes H=1.5x, V=1.428x (was 1.5x H, 0.93x V).
#define TOP_X          0
#define TOP_Y          0
#define TOP_W          480
#define TOP_H          220

#define BOT_X          0
#define BOT_Y          220
#define BOT_W          480
#define BOT_H          100

// Translate source-image coords to on-screen coords for text-slot placement.
// Top uses the 1.5x both-axes scale.
static inline int sx     (int srcX) { return (srcX * TOP_W) / SRC_W; }       // *1.5
static inline int sy_top (int srcY) { return TOP_Y + (srcY * TOP_H) / SRC_TOP_H; } // *1.5
// Bottom uses 1.5x H, ~0.93x V (BOT_H / SRC_BOT_H = 65/70).
static inline int sy_bot (int srcY) { return BOT_Y + (srcY * BOT_H) / SRC_BOT_H; }

// ============================================================================
//  Hardware
// ============================================================================
static Arduino_ESP32QSPI *jc_bus   = nullptr;
static Arduino_AXS15231B *jc_panel = nullptr;
static Arduino_Canvas    *gfx      = nullptr;

// Cyclic-screen IDs
enum : int { SCR_MINER = 0, SCR_CLOCK = 1, SCR_GLOBAL = 2, SCR_PRICE = 3, SCR_COUNT = 4 };
enum : int { LOWER_POOL = 1, LOWER_FEES = 2 };
static int lowerScreen = LOWER_POOL;

static void toggleBottomScreen() {
    lowerScreen = (lowerScreen == LOWER_POOL) ? LOWER_FEES : LOWER_POOL;
}

// ============================================================================
//  BACKGROUND CACHE — the v0.8 transparency story
// ============================================================================
//
// bg_cache holds a fully-rendered framebuffer-shaped copy of the current
// (cyclic, lower) artwork. It lives in PSRAM (~307 KB) and is regenerated
// only when the combo changes. The canvas is restored from it on each draw
// pass, so text overlays appear over real artwork instead of black boxes.
//
static uint16_t *bg_cache       = nullptr;

// v0.8.21 PRE-RENDER SLABS — eliminate the per-screen-change scaling cost.
//
// jc_scaleIntoBgCache() reads the PROGMEM source art and computes every
// destination pixel via nearest-neighbor math (~100-200 ms per top art).
// That cost is paid on every tap-to-switch.
//
// These slabs hold the SCALED top and bottom artwork in PSRAM, populated
// lazily on first use. On screen-change we memcpy the slab into bg_cache
// (a few hundred microseconds, vs ~100ms of scaling).
//
// Memory budget at JC_W=480: top slab is 480x220 RGB565 = 211200 B per
// variant (4 variants = ~824 KB). Bottom slab is 480x100 = 96000 B per
// variant (2 variants = ~188 KB). Total ~1 MB PSRAM, well within budget
// of our 8 MB OPI PSRAM allocation.
#define NUM_TOP_VARIANTS  4
#define NUM_BOT_VARIANTS  2
static uint16_t *prerender_top[NUM_TOP_VARIANTS] = {nullptr, nullptr, nullptr, nullptr};
static uint16_t *prerender_bot[NUM_BOT_VARIANTS] = {nullptr, nullptr};
static int cached_cycle = -1;
static int cached_lower = -1;

// ============================================================================
//  Flash mitigation — "only flush when displayed values change"
//
//  Each cyclic screen builds a small snapshot String of every value it draws.
//  At end of screen function we compare snapshot to its cached counterpart;
//  if identical, gfx->flush() is SKIPPED (no panel push, no visible flash).
//  When the background combo (cycle, lower) changes we always force a flush.
//
//  Snapshot is per-screen, indexed by SCR_* IDs. 4 slots, one per cyclic.
// ============================================================================
static String last_snapshot[SCR_COUNT];
static int    last_snapshot_cycle = -1;     // tracks (cycle,lower) at last flush
static int    last_snapshot_lower = -1;

// Returns true if a flush is needed (snapshot changed OR bg changed).
// On true, caller calls gfx->flush() and we update the cached snapshot.
// LAYOUT/FONT/COLOR tweaks aren't invisible until data changes. Cost is
// a periodic mini-flash but it makes development iterations trustworthy.
#define JC_MAX_FLUSH_GAP_MS  5000
static uint32_t last_flush_ms[SCR_COUNT] = {0, 0, 0, 0};

static bool jc_shouldFlush(int scr, const String &snap)
{
    bool bg_changed = (cached_cycle != last_snapshot_cycle) ||
                      (cached_lower != last_snapshot_lower);
    bool data_changed = (last_snapshot[scr] != snap);
    uint32_t now = millis();
    bool stale = (now - last_flush_ms[scr]) > JC_MAX_FLUSH_GAP_MS;
    if (bg_changed || data_changed || stale) {
        last_snapshot[scr] = snap;
        last_snapshot_cycle = cached_cycle;
        last_snapshot_lower = cached_lower;
        last_flush_ms[scr] = now;
        return true;
    }
    return false;
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

// Restore a rectangular region of the canvas from the bg cache.
// This is what we call before drawing new text into a slot.
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
    // Scale into the slab using the same NN math as jc_scaleIntoBgCache,
    // but writing into the slab buffer directly.
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

// memcpy a top slab into bg_cache (replaces jc_scaleIntoBgCache for top).
// Assumes the slab is already populated. Source is contiguous TOP_W rows,
// destination region in bg_cache also has TOP_W = JC_W width, so a single
// memcpy per row works.
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

static void jc_ofr_drawPixel(int32_t x, int32_t y, uint16_t c) {
    if (gfx) gfx->writePixel((int16_t)x, (int16_t)y, c);
}
static void jc_ofr_drawHLine(int32_t x, int32_t y, int32_t w, uint16_t c) {
    if (gfx) gfx->writeFastHLine((int16_t)x, (int16_t)y, (int16_t)w, c);
}
static void jc_ofr_startWrite(void) { if (gfx) gfx->startWrite(); }
static void jc_ofr_endWrite(void)   { if (gfx) gfx->endWrite();   }

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

static void jc_pollTouch()
{
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
//  Drawing helpers — "dyn" variants restore bg from cache before painting
// ============================================================================
static void jc_textAt(int x, int y, uint16_t color, uint8_t size, const char *fmt, ...) {
    char buf[96]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    gfx->setTextColor(color); gfx->setTextSize(size); gfx->setCursor(x, y); gfx->print(buf);
}

// Erase via bg-cache restore, then paint Arduino_GFX built-in font text.
static void jc_dynText(int x, int y, int w, int h, uint16_t color, uint8_t size,
                       const char *fmt, ...) {
    char buf[96]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    jc_restoreRect(x, y, w, h);
    gfx->setTextColor(color); gfx->setTextSize(size); gfx->setCursor(x, y); gfx->print(buf);
}

// Erase via bg-cache restore, then paint OpenFontRender TTF text.
static void jc_dynOfr(JcFont face, int x, int y, int w, int h, uint16_t color,
                      uint16_t size_px, const char *fmt, ...) {
    if (!render_loaded) return;
    char buf[64]; va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    jc_restoreRect(x, y, w, h);
    jc_loadFont(face);
    render.setFontSize(size_px);
    render.setFontColor(color);
    render.setCursor(x, y);
    render.printf("%s", buf);
}

// Direct OFR draw without erase (for one-shot static labels on loading/setup)
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

// ============================================================================
//  Background renderer — fills bg_cache then blits to canvas
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

static void jc_renderBackground(int cyc, int lo)
{
    if (!gfx || !bg_cache) return;
    Serial.printf("[jc3248w535] bg render: cycle=%d lower=%d\n", cyc, lo);

    // 1) Ensure pre-rendered slabs exist; lazy-allocate on first request.
    bool top_ok = jc_ensureTopSlab(cyc);
    bool bot_ok = jc_ensureBotSlab(lo);

    // 2) Compose bg_cache from slabs (fast memcpy), or fall back to the
    //    scaling path if a slab allocation failed (PSRAM exhausted).
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

    // 3) Blit the whole cache into the canvas. This is the only "big"
    //    canvas write per screen-change; subsequent text updates only
    //    touch small rects.
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
    // v0.8.26: when ARDUINO_USB_CDC_ON_BOOT=1 and ARDUINO_USB_MODE=1 are set,
    // `Serial` is an HWCDC instance attached to the built-in USB-Serial/JTAG.
    // HWCDC::write() BLOCKS (default 100 ms timeout per call) when the TX
    // ring buffer fills and no USB host is draining it. Multiple println()
    // calls back-to-back on a headless boot can stack timeouts and stall
    // init for many seconds — which is what was making the screen stay
    // black until picocom attached and started reading bytes.
    //
    // Setting the timeout to 0 makes write() drop data silently if no host
    // is reading. With a host attached, behavior is unchanged.
#if defined(ARDUINO_USB_CDC_ON_BOOT) && defined(ARDUINO_USB_MODE)
    Serial.setTxTimeoutMs(0);
#endif

    // v0.8.25: HWCDC Serial.flush() BLOCKS forever if no USB host is reading
    // the buffer (e.g., normal headless boot with no serial monitor open).
    // Replaced with non-blocking output (no flush). 3 banners give a host
    // enough material to recognize the firmware if it connects late.
    for (int i = 0; i < 3; i++) {
        Serial.println(F("[jc3248w535] init v0.8.27 ##############"));
    }
    delay(50);   // brief gap to let USB-CDC enumerate if a host is plugged in
    Serial.printf("[jc3248w535] free heap=%u psram=%u\n",
                  ESP.getFreeHeap(), ESP.getFreePsram());
    pinMode(LCD_BL, OUTPUT); digitalWrite(LCD_BL, HIGH);

    jc_bus   = new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
    jc_panel = new Arduino_AXS15231B(jc_bus, GFX_NOT_DEFINED, 0, false, 320, 480);
    gfx      = new Arduino_Canvas(320, 480, jc_panel, 0, 0, 0);
    if (!gfx || !gfx->begin()) { Serial.println(F("[jc3248w535] gfx->begin() FAILED")); return; }
    gfx->setRotation(1); gfx->fillScreen(BLACK); gfx->flush();

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
    pinMode(TOUCH_INT, INPUT_PULLUP);
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);  delay(50);
    digitalWrite(TOUCH_RST, HIGH); delay(150);
    Wire.beginTransmission(TOUCH_I2C_ADDR);
    Serial.printf("[jc3248w535] touch %s @ 0x%02X\n",
                  (Wire.endTransmission() == 0) ? "OK" : "MISSING", TOUCH_I2C_ADDR);
    Serial.printf("[jc3248w535] layout top=%dx%d bot=%dx%d (fill width)\n",
                  TOP_W, TOP_H, BOT_W, BOT_H);
}

void jc3248w535_AlternateScreenState(void) {
    static bool on = true; on = !on;
    digitalWrite(LCD_BL, on ? HIGH : LOW);
}

void jc3248w535_AlternateRotation(void) {
    int r = (gfx->getRotation() == 1) ? 3 : 1;
    gfx->setRotation(r); gfx->fillScreen(BLACK); gfx->flush();
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
    // Init screen is 320x170, scale to 480x255 directly via canvas writer.
    // (We could use the cache here too; doesn't matter, it's a one-shot.)
    for (int dy = 0; dy < 255; dy++) {
        int sy_s = (dy * 170) / 255;
        const unsigned short *row = initScreen + sy_s * 320;
        for (int dx = 0; dx < 480; dx++) {
            int sx_s = (dx * 320) / 480;
            gfx->writePixel(dx, dy, pgm_read_word(&row[sx_s]));
        }
    }
    jc_textAt(sx(24), sy_top(147), BLACK, 1, "v%s", CURRENT_VERSION);
    jc_textAt(8, 280, WHITE, 2, "Initializing NerdMiner...");
    jc_textAt(8, 304, DARKGREY, 1, "Guition JC3248W535 / 8MB PSRAM / 16MB Flash");
    gfx->flush();
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
    jc_textAt(8,  306, WHITE,  1, "Then browse to http://192.168.4.1");
    gfx->flush();
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

// ============================================================================
//  Cyclic screens
// ============================================================================
static void jc3248w535_MinerScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
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
    jc_dynOfr(JcFont::Digital, sx(234), sy_top(142), sx(290)-sx(234), 26,
              BLACK, 22, "%s", data.totalMHashes.c_str());

    // Temp + time at top of art (harmonized to Global Stats toolbar style:
    // Arduino_GFX built-in font size 2, BLACK).
    // Temp uses degree symbol (0xF7 in many Adafruit GFX fonts ≈ '°').
    // If 0xF7 prints as a different glyph on your panel, switch to "%sC"
    // (plain ASCII) or draw a small circle outline geometrically.
    // Temp: built-in size 2 numeric value, then a small filled-circle degree
    // mark hand-drawn (\xF7 rendered as a math symbol on this font).
    // Slot covers number + a few px on the right for the circle.
    jc_dynText(sx(225), sy_top(4), 40, 14, BLACK, 2, "%s", data.temp.c_str());
    {
        // Compute right edge of the rendered temp string to place the
        // degree symbol just after it. Each char in size-2 built-in font
        // is ~12 screen-px wide.
        int chars = (int)strlen(data.temp.c_str());
        int dot_x = sx(225) + chars * 12 + 2;   // +2 px gap
        int dot_y = sy_top(4) + 2;              // slightly above baseline
        // Draw a small circle (degree mark)
        gfx->drawCircle(dot_x, dot_y, 2, BLACK);
    }
    // Time: built-in size 2 at sx(249). v0.8.16.1's sx(253) had a tiny
    // bit of room on the right; nudged 4 px left.
    jc_dynText(sx(249), sy_top(4), sx(315)-sx(249), 14, BLACK, 2,
               "%s",  data.currentTime.c_str());

    // Bottom panel
    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    pool_data pds = getPoolData();
    coin_data cds = getCoinData(mElapsed);
    String snap = data.currentHashRate + "|" + data.templates + "|" + data.bestDiff
                + "|" + data.completedShares + "|" + data.valids + "|" + data.timeMining
                + "|" + data.totalMHashes + "|" + data.temp + "|" + data.currentTime
                + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + pds.workersHash)
                    : (cds.fastestFee + "/" + cds.economyFee + "/" + cds.minimumFee));
    if (jc_shouldFlush(SCR_MINER, snap)) gfx->flush();

    Serial.printf(">>> Miner: %s share(s)  hash %s KH/s\n",
                  data.completedShares.c_str(), data.currentHashRate.c_str());
    jc_pollTouch();
}

static void jc3248w535_ClockScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_ensureBackground(SCR_CLOCK, lowerScreen);
    clock_data data = getClockData(mElapsed);

    // Big clock — v0.8.7: right-aligned to right half of art (32 px right
    //                     margin), color WHITE, font NotoBold.
    jc_loadFont(JcFont::NotoBold);
    render.setFontSize(78);
    uint16_t cw = render.getTextWidth(data.currentTime.c_str());
    int cx = JC_W - (int)cw - 32; if (cx < 4) cx = 4;
    jc_dynOfr(JcFont::NotoBold, cx, sy_top(40), JC_W - cx, 90, WHITE, 78,
              "%s", data.currentTime.c_str());

    // Hashrate lower-left
    jc_dynOfr(JcFont::Digital, sx(19), sy_top(128), sx(120)-sx(19), 38,
              BLACK, 34, "%s", data.currentHashRate.c_str());

    // Block height lower-right — font: DigitalNumbers @ 26 px (TTF via OFR)
    jc_dynOfr(JcFont::Digital, sx(180) - 9, sy_top(140), sx(290)-(sx(180)-9), 26,
              BLACK, 26, "%s", data.blockHeight.c_str());

    // BTC price top of art
    jc_dynText(sx(195) + 15, sy_top(7), sx(315)-(sx(195)+15), 14, BLACK, 2,
               "%s", data.btcPrice.c_str());

    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    pool_data pds = getPoolData();
    coin_data cds = getCoinData(mElapsed);
    String snap = data.currentTime + "|" + data.btcPrice + "|" + data.currentHashRate
                + "|" + data.blockHeight + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + pds.workersHash)
                    : (cds.fastestFee + "/" + cds.economyFee + "/" + cds.minimumFee));
    if (jc_shouldFlush(SCR_CLOCK, snap)) gfx->flush();

    Serial.printf(">>> Clock %s  BTC %s\n", data.currentTime.c_str(), data.btcPrice.c_str());
    jc_pollTouch();
}

static void jc3248w535_GlobalHashScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_ensureBackground(SCR_GLOBAL, lowerScreen);
    coin_data data = getCoinData(mElapsed);
    clock_data ctm = getClockData(mElapsed);

    jc_dynText(sx(195) + 15, sy_top(7), 100, 14, BLACK, 2, "%s",
               data.btcPrice.c_str());
    //          Both share sy_top(7) so already on same horizontal plane.
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
    String snap = data.btcPrice + "|" + ctm.currentTime + "|" + data.halfHourFee
                + "|" + data.netwrokDifficulty + "|" + data.globalHashRate
                + "|" + data.blockHeight + "|" + String((int)(data.progressPercent * 10))
                + "|" + data.remainingBlocks + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + pds.workersHash)
                    : (data.fastestFee + "/" + data.economyFee + "/" + data.minimumFee));
    if (jc_shouldFlush(SCR_GLOBAL, snap)) gfx->flush();

    Serial.printf(">>> Global BTC %s  net %s\n",
                  data.btcPrice.c_str(), data.globalHashRate.c_str());
    jc_pollTouch();
}

static void jc3248w535_PriceScreen(unsigned long mElapsed)
{
    if (!gfx) { jc_pollTouch(); return; }
    jc_ensureBackground(SCR_PRICE, lowerScreen);
    clock_data data = getClockData(mElapsed);

    //          Was NotoBold @ 24, sy_top(0). Now built-in size 2, sy_top(7).
    //          X (sx(220)) preserved per user "x-axis seems good".
    jc_dynText(sx(220), sy_top(7), sx(315) - sx(220), 14, BLACK, 2,
               "%s", data.currentTime.c_str());

    // BIG BTC price — headline of this screen
    jc_loadFont(JcFont::NotoBold);
    render.setFontSize(64);
    uint16_t pw = render.getTextWidth(data.btcPrice.c_str());
    int px = JC_W - (int)pw - 16;
    if (px < 4) px = 4;
    jc_dynOfr(JcFont::NotoBold, px, sy_top(46), JC_W - px, 72, WHITE, 64,
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
    String snap = data.btcPrice + "|" + data.currentTime + "|" + data.currentHashRate
                + "|" + data.blockHeight + "|L" + String(lowerScreen) + "|"
                + (lowerScreen == LOWER_POOL
                    ? (pds.bestDifficulty + "/" + String(pds.workersCount) + "/" + pds.workersHash)
                    : (cds.fastestFee + "/" + cds.economyFee + "/" + cds.minimumFee));
    if (jc_shouldFlush(SCR_PRICE, snap)) gfx->flush();

    Serial.printf(">>> Price BTC %s  hash %s KH/s\n",
                  data.btcPrice.c_str(), data.currentHashRate.c_str());
    jc_pollTouch();
}

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
    static uint32_t last = 0;
    uint32_t now = millis();
    uint32_t period = 0;
    switch (mMonitor.NerdStatus) {
        case NM_waitingConfig: period = 0;    break;
        case NM_Connecting:    period = 500;  break;
        case NM_hashing:       period = 1500; break;
        default:               period = 0;
    }
    if (period == 0) { digitalWrite(LCD_BL, HIGH); return; }
    if (now - last >= period) {
        last = now;
        digitalWrite(LCD_BL, LOW); delay(20); digitalWrite(LCD_BL, HIGH);
    }
}

// ============================================================================
//  Driver table
// ============================================================================
CyclicScreenFunction jc3248w535CyclicScreens[] = {
    jc3248w535_MinerScreen,
    jc3248w535_ClockScreen,
    jc3248w535_GlobalHashScreen,
    jc3248w535_PriceScreen,
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
