// =============================================================================
//  Guition JC3248W535 display driver for NerdMiner v2  (v0.8.11)
// =============================================================================
//
//  Stack: Arduino_GFX 1.6.0 + OpenFontRender 1.2 + canonical NerdMiner art
//
//  v0.8.11 changes vs v0.8.10 — toolbar harmonization round:
//    Canonical toolbar style (from Global Stats) is now applied to all
//    screens: Arduino_GFX built-in font @ size 2, y = sy_top(7), BLACK.
//
//    * Global Stats:
//        - Global hashrate RIGHT 1 more character (+14 px). Total +75 from sx(190).
//        - Toolbar unchanged (it is the canonical reference now).
//    * Price screen:
//        - Toolbar time: NotoBold@24 sy_top(0) -> built-in size 2 sy_top(7).
//          X (sx(220)) preserved per user feedback.
//    * Miner screen:
//        - Toolbar temp + time: NotoBold@24 sy_top(0) -> built-in size 2
//          sy_top(7) (matches Global). Temp moved RIGHT (sx(195) -> sx(214))
//          to sit closer to time.
//    * Clock screen toolbar already matches the canonical style — no change.
//
//  v0.8.10 changes vs v0.8.9 — minor position pass + flush stale fix:
//    * Miner:
//        - Hashrate: LEFT one character (~28 px at Digital 50px).
//        - Uptime:   RIGHT one character (~7 px at Digital 16px).
//    * Clock:
//        - Big clock font 74 -> 78 ("a bit larger").
//    * Global Stats:
//        - Global hashrate: RIGHT 3 characters (~42 px at Digital 24).
//        - Toolbar time:   font NotoBold 24 -> built-in size 2 to match
//                          toolbar price. Both already on sy_top(7).
//    * jc_shouldFlush(): force a flush every 5s even if snapshot unchanged.
//      Prevents the "I uploaded but don't see my layout changes" bug where
//      a value-snapshot match was suppressing the flush after layout edits.
//
//  v0.8.9 changes vs v0.8.8 — four-screen polish round:
//    * Miner:
//        - Uptime:   RIGHT 12 more (now +24 total from sx(180) baseline).
//        - Toolbar:  temp+time brought CLOSER (time x sx(255)->sx(245),
//                    -10) and RAISED 1 (sy_top(1)->sy_top(0)).
//        - Hashrate: RIGHT 10 more + DOWN 5 more (now x=sx(50)-23,
//                    y=sy_top(110)).
//    * Clock:
//        - Big clock font 70 -> 74 (slightly larger).
//        - Toolbar BTC: RIGHT one char (+12) + DOWN 2 (sy 5 -> 7).
//        - Block height: LEFT 4 more (now sx(180)-9). FONT = DigitalNumbers
//          (TTF via OpenFontRender) @ 26 px, BLACK.
//    * Global Stats:
//        - Global hashrate: RIGHT 14 more (one char) to space from "EH/s".
//        - Toolbar BTC: matched Clock toolbar (RIGHT +12, DOWN 2).
//        - Toolbar time: y aligned to toolbar BTC (sy_top(7)) so both share
//          the same horizontal axis.
//    * Price:
//        - Toolbar time: raised 1 (sy_top(1)->sy_top(0)) matching Miner/Clock.
//
//  v0.8.8 changes vs v0.8.7 — three-screen polish:
//    * Miner screen:
//        - Hashrate:       LEFT 5 + DOWN 5 (equal). x=sx(50)-33, y=sy_top(105).
//        - Block templates RIGHT 4 (only that one of the right-column stats).
//        - Uptime:         RIGHT 12 (~2 chars at Digital 16).
//        - Worker count:   font 24 -> 28 (one step bigger).
//    * Clock screen:
//        - BTC top of art: RIGHT 3 + DOWN 2.
//        - Block height:   DOWN 3 (was at sy 137, now 140).
//        - Hashrate:       NOW IDENTICAL to Price screen hashrate
//                          (sx(19), sy_top(128), slot 38, BLACK, Digital 34).
//    * Global Stats screen:
//        - Global hashrate: RIGHT 5 + DOWN 3 + color WHITE -> BLACK.
//        - Medium fee + Difficulty: DOWN 3 + color light-blue -> WHITE.
//        - Toolbar price: RIGHT 3 + DOWN 2.
//        - Toolbar time:  DOWN 2 + switched to NotoBold @ 24 (matches Price
//                         header time and Clock toolbar time).
//
//  v0.8.7 changes vs v0.8.6 — multi-screen polish:
//    * Price screen:
//        - Time font 22 -> 24 (slightly bigger)
//    * Miner screen:
//        - Header temp + time: built-in size-1 -> NotoBold @ 24 (matches
//          Price screen header time; smoother than DigitalNumbers/T-HMI)
//        - Uptime: moved LEFT (sx(245) -> sx(180)) and UP 2 (sy 108 -> 106)
//        - Hashrate: moved LEFT one character (~28 screen-px) and DOWN 5
//                    (sy 95 -> 100). Erase rect widened left.
//    * Clock screen:
//        - Hashrate: DOWN 5 (sy 118 -> 123), WHITE -> BLACK
//        - Block height: LEFT 5, DOWN 2 (sy 135 -> 137), WHITE -> BLACK
//        - Big clock: was centered cyan DigitalNumbers 70px;
//                     now right-aligned (32px right margin) WHITE NotoBold
//
//  v0.8.6 changes vs v0.8.5:
//    * Price screen:
//        - Time:  literal up 4 (sy_top 5 -> 1) to center on header bar.
//                 Slot enlarged to absorb the higher top so 22px font
//                 doesn't get clipped.
//    * Miner screen:
//        - Hashrate:    font 65 -> 50px, color CYAN -> BLACK.
//        - Uptime:      switched from built-in size-1 to Digital @ 16px,
//                       and lowered 4px (sy_top 104 -> 108).
//        - Total MH:    switched from built-in size-2 to Digital @ 22px,
//                       and lowered 4px (sy_top 138 -> 142).
//    * Worker count tweak DEFERRED — awaiting exact target Y from user.
//
//  v0.8.5 changes vs v0.8.4 — Price screen polish round 3:
//    * BTC headline:           raised 5 more px (sy_top 51 -> 46)
//    * Hashrate:               lowered 2 more px (sy_top 126 -> 128)
//    * Time (top-right):       raised 1px (sy_top 6 -> 5), font 18 -> 22 px
//                              (slot height also bumped 22 -> 26)
//    * Bottom POOL workers ct: lowered 2 more px (+6 total from baseline)
//
//  v0.8.4 changes vs v0.8.3 — Price screen polish round 2:
//    * BTC headline:           raised another 2px  (sy_top 53 -> 51)
//    * Hashrate:               lowered another 2px (sy_top 124 -> 126)
//    * Time (top-right):       lowered another 2px (sy_top 4 -> 6)
//                              + switched from chunky built-in font to
//                              smoother NotoSans Bold (TTF @ 18px) so the
//                              clock looks less dotty.
//    * Bottom POOL panel:      workers count ONLY lowered another 2px
//                              (best-diff and workersHash slots untouched).
//
//  v0.8.3 changes vs v0.8.2 — layout polish per visual feedback:
//    * Price screen:
//        - BTC headline raised 2px
//        - Time lowered 1px
//        - Hashrate lowered 2px, color BLACK, font size 30 -> 34
//        - Block height lowered 2px
//    * Bottom panel (both POOL and FEES variants):
//        - All values lowered 2 screen-px
//        - Switched to DigitalNumbers font at 24px (was built-in size 2)
//        - Slot height bumped to 28 to accommodate taller glyphs
//
//  v0.8.2 changes vs v0.8 (v0.8.1 was reverted - bypassing canvas broke
//                         coordinates; the partial-flush experiment failed):
//    * FLASH-REDUCTION ("only flush when values changed"):
//        Tracks a per-screen snapshot of all displayed strings. At the end
//        of each cyclic-screen function, compares new snapshot to cached.
//        If nothing changed -> SKIP gfx->flush() entirely (no flash that
//        frame). If anything changed -> flush as before. Worst case = v0.8.
//        Best case: at idle / between value updates, panel is silent.
//
//    * BOUNDARY 255 -> 220 between top and bottom panels:
//        Top:    320x170 src -> 480x220 (H=1.5x, V=1.294x; slight V squish)
//        Bottom: 320x70  src -> 480x100 (H=1.5x, V=1.428x; slight V stretch)
//        Touch threshold moves to y > 220 to match the new boundary.
//
//    * PRICE SCREEN: BTC headline right-justified and WHITE (was centered cyan)
//
//  v0.8 changes vs v0.7.3:
//    * REAL TRANSPARENT TEXT OVERLAY:
//        - Background is rendered once into a dedicated PSRAM bg-cache
//          buffer (480x320 RGB565 = ~307 KB in PSRAM).
//        - At driver init we copy that buffer into the canvas.
//        - On each text update, the per-field "erase" copies the matching
//          slice of the bg-cache back into the canvas (restoring artwork)
//          BEFORE painting new text. No more black rectangles hiding the
//          artwork.
//    * FILL-WIDTH SCALING (Option C):
//        - Top image scaled 1.5x both axes -> 480x255 (was 400x213)
//        - Bottom image scaled 1.5x H, 0.93x V -> 480x65 (was 400x88)
//          (Slight 7% vertical squish on the bottom panel only, so the
//           combined height matches the 320-px screen exactly.)
//        - No side margins; image fills the whole screen.
//    * FOOTER HINT REMOVED:
//        - The "tap top: next screen / tap bottom: workers <-> fees" line
//          is gone. Total layout: top 0..255, bottom 255..320 (65px). The
//          full panel is artwork now.
//    * Flashing should disappear: no per-second fillScreen(BLACK), and the
//      per-field erase is a sub-rect bg-cache memcpy (~few KB) rather than
//      a black fill on the canvas.
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
// v0.8.2: top shorter (220px), bottom taller (100px) per visual feedback.
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
// v0.8.10: also force a flush every N ms regardless of snapshot, so
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

    // 1) Scale top + bottom INTO the bg cache.
    jc_scaleIntoBgCache(jc_topArtFor(cyc),
                         SRC_W, SRC_TOP_H, TOP_X, TOP_Y, TOP_W, TOP_H);
    jc_scaleIntoBgCache(jc_bottomArtFor(lo),
                         SRC_W, SRC_BOT_H, BOT_X, BOT_Y, BOT_W, BOT_H);

    // 2) Blit the whole cache into the canvas. This is the only "big" write
    //    per screen-change; subsequent text updates only touch small rects.
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
    Serial.println(F("[jc3248w535] init v0.8.11"));
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
    // v0.8.2: invalidate snapshot cache so next render is forced to flush
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
// v0.8.3: bottom-panel fields use DigitalNumbers (was built-in font),
//         lowered 2 screen-px, font size up to 24px ("a little larger").
//         Erase rect height bumped to 28 to cover taller glyphs.
static void jc_drawLowerPool(unsigned long /*mElapsed*/)
{
    pool_data p = getPoolData();
    jc_dynOfr(JcFont::Digital, sx(5),   sy_bot(34) + 2, sx(80) - sx(5),    28,
              BLACK, 24, "%s", p.bestDifficulty.c_str());
    // v0.8.4: workers count ONLY lowered another 2px (other two unchanged)
    // v0.8.5: workers count lowered another 2px (total +6 from baseline)
    // v0.8.8: font size 24 -> 28 (one step bigger); slot height 28 -> 32.
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
    // v0.8.6: smaller font (65 -> 50) and color CYAN -> BLACK.
    // v0.8.7: moved LEFT one character (~28 screen-px at Digital 50px)
    //         and DOWN 5 (sy_top(95) -> sy_top(100)). Slot widened LEFT.
    // v0.8.8: moved LEFT 5 + DOWN 5 (equal). x: sx(50)-28 -> sx(50)-33.
    // v0.8.9: moved RIGHT 10 and DOWN 5 more. x: sx(50)-33 -> sx(50)-23.
    // v0.8.10: moved LEFT one character (~28px at Digital 50px). x -> sx(50)-51.
    jc_dynOfr(JcFont::Digital, sx(50) - 51, sy_top(110), sx(180)-(sx(50)-51), 44,
              BLACK, 50, "%s", data.currentHashRate.c_str());

    // Stats column on the right side of art (templates / bestDiff / shares)
    // v0.8.8: block templates only moved RIGHT 4 px (other two unchanged).
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
    // v0.8.6: enlarged from built-in size-1 (~7px) to Digital @ 16px,
    //         and moved down 4px (sy_top(104) -> sy_top(108)).
    // v0.8.7: moved LEFT a large amount (sx(245) -> sx(180)) and slightly
    //         up (sy_top(108) -> sy_top(106)).
    // v0.8.8: moved RIGHT 2 characters (~12 px at Digital 16).
    // v0.8.9: moved RIGHT 2 more characters (+24 total from sx(180)).
    // v0.8.10: moved RIGHT one more character (~7px at Digital 16). +31 total.
    jc_dynOfr(JcFont::Digital, sx(180) + 31, sy_top(106), sx(315)-(sx(180)+31), 20,
              WHITE, 16, "%s", data.timeMining.c_str());

    // Total MHashes ("million hashes")
    // v0.8.6: enlarged from built-in size-2 (~14px) to Digital @ 22px,
    //         and moved down 4px (sy_top(138) -> sy_top(142)).
    jc_dynOfr(JcFont::Digital, sx(230), sy_top(142), sx(290)-sx(230), 26,
              BLACK, 22, "%s", data.totalMHashes.c_str());

    // Temp + time at top of art
    // v0.8.11: harmonized to Global Stats toolbar font/y (built-in size 2,
    //          sy_top(7)). Was NotoBold @ 24, sy_top(0).
    //          Temp also moved RIGHT closer to time: sx(195) -> sx(214)
    //          (leaves ~10 screen-px gap between "42C" and time start).
    jc_dynText(sx(214), sy_top(7), 50, 14, BLACK, 2, "%sC", data.temp.c_str());
    jc_dynText(sx(245), sy_top(7), sx(315)-sx(245), 14, BLACK, 2,
               "%s",  data.currentTime.c_str());

    // Bottom panel
    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    // v0.8.2: skip flush if nothing changed since last render
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
    // v0.8.9:  font 70 -> 74.
    // v0.8.10: font 74 -> 78 ("a bit larger"). Slot 84 -> 90.
    jc_loadFont(JcFont::NotoBold);
    render.setFontSize(78);
    uint16_t cw = render.getTextWidth(data.currentTime.c_str());
    int cx = JC_W - (int)cw - 32; if (cx < 4) cx = 4;
    jc_dynOfr(JcFont::NotoBold, cx, sy_top(40), JC_W - cx, 90, WHITE, 78,
              "%s", data.currentTime.c_str());

    // Hashrate lower-left
    // v0.8.7: lowered 5px (sy_top(118) -> sy_top(123)), WHITE -> BLACK.
    // v0.8.8: made IDENTICAL to Price screen hashrate (position + font):
    //         sy_top(123) -> sy_top(128), slot 32 -> 38, font 32 -> 34.
    jc_dynOfr(JcFont::Digital, sx(19), sy_top(128), sx(120)-sx(19), 38,
              BLACK, 34, "%s", data.currentHashRate.c_str());

    // Block height lower-right — font: DigitalNumbers @ 26 px (TTF via OFR)
    // v0.8.7: moved LEFT 5px, DOWN 2, WHITE -> BLACK.
    // v0.8.8: lowered another 3px (sy 137 -> 140).
    // v0.8.9: moved slightly LEFT another 4px. x: sx(180)-5 -> sx(180)-9.
    jc_dynOfr(JcFont::Digital, sx(180) - 9, sy_top(140), sx(290)-(sx(180)-9), 26,
              BLACK, 26, "%s", data.blockHeight.c_str());

    // BTC price top of art
    // v0.8.8: moved slightly RIGHT 3 + DOWN 2 (sy_top(3) -> sy_top(5)).
    // v0.8.9: moved RIGHT one more character (+12) and DOWN 2 more.
    //         x: sx(195)+3 -> sx(195)+15; y: sy_top(5) -> sy_top(7).
    jc_dynText(sx(195) + 15, sy_top(7), sx(315)-(sx(195)+15), 14, BLACK, 2,
               "%s", data.btcPrice.c_str());

    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    // v0.8.2: skip flush if nothing changed
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

    // v0.8.8: toolbar price moved slightly RIGHT 3 + DOWN 2.
    // v0.8.9: matched to Clock toolbar: RIGHT one more char (+12) + DOWN 2.
    //         x: sx(195)+3 -> sx(195)+15; y: sy_top(5) -> sy_top(7).
    jc_dynText(sx(195) + 15, sy_top(7), 100, 14, BLACK, 2, "%s",
               data.btcPrice.c_str());
    // v0.8.10: changed time font to match price (built-in size 2, was NotoBold@24).
    //          Both share sy_top(7) so already on same horizontal plane.
    jc_dynText(sx(265), sy_top(7), sx(315)-sx(265), 14, BLACK, 2,
               "%s", ctm.currentTime.c_str());

    // v0.8.8: medium fee + difficulty lowered 3px, color light-blue -> WHITE.
    jc_dynText(sx(240), sy_top(53), sx(315)-sx(240), 18, WHITE, 2,
               "%s", data.halfHourFee.c_str());
    jc_dynText(sx(240), sy_top(88), sx(315)-sx(240), 18, WHITE, 2,
               "%s", data.netwrokDifficulty.c_str());

    // v0.8.9:  RIGHT one char (~14 px) for space from "EH/s" label.
    // v0.8.10: RIGHT 3 more characters (~42 px at Digital 24). Total +61 from sx(190).
    // v0.8.11: RIGHT 1 more character (~14 px at Digital 24). Total +75.
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

    // v0.8.2: skip flush if nothing changed
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

    // v0.8.11: harmonized to Global Stats toolbar font/y.
    //          Was NotoBold @ 24, sy_top(0). Now built-in size 2, sy_top(7).
    //          X (sx(220)) preserved per user "x-axis seems good".
    jc_dynText(sx(220), sy_top(7), sx(315) - sx(220), 14, BLACK, 2,
               "%s", data.currentTime.c_str());

    // BIG BTC price — headline of this screen
    jc_loadFont(JcFont::NotoBold);
    render.setFontSize(64);
    // v0.8.2: right-justified with a 16px right margin, color WHITE.
    // v0.8.3: raised 2px (sy_top(55) -> sy_top(53)).
    // v0.8.4: raised another 2px (sy_top(53) -> sy_top(51)).
    // v0.8.5: raised 5 more px (sy_top(51) -> sy_top(46)).
    uint16_t pw = render.getTextWidth(data.btcPrice.c_str());
    int px = JC_W - (int)pw - 16;
    if (px < 4) px = 4;
    jc_dynOfr(JcFont::NotoBold, px, sy_top(46), JC_W - px, 72, WHITE, 64,
              "%s", data.btcPrice.c_str());

    // v0.8.3: hashrate lowered 2px (sy_top(122) -> sy_top(124)),
    //         BLACK (was WHITE), font size 30 -> 34
    // v0.8.4: lowered another 2px (sy_top(124) -> sy_top(126))
    // v0.8.5: lowered another 2px (sy_top(126) -> sy_top(128))
    jc_dynOfr(JcFont::Digital, sx(19), sy_top(128), sx(120)-sx(19), 38,
              BLACK, 34, "%s", data.currentHashRate.c_str());
    // v0.8.3: block height lowered 2px (sy_top(135) -> sy_top(137))
    jc_dynOfr(JcFont::Digital, sx(180), sy_top(137), sx(290)-sx(180), 24,
              WHITE, 24, "%s", data.blockHeight.c_str());

    if (lowerScreen == LOWER_POOL) jc_drawLowerPool(mElapsed);
    else                           jc_drawLowerFees(mElapsed);

    // v0.8.2: skip flush if nothing changed
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
