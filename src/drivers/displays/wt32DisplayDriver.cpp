#include "displayDriver.h"

#ifdef WT32_DISPLAY

#define LGFX_USE_V1

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>

#include "monitor.h"
#include "drivers/storage/storage.h"
#include "wManager.h"
#include "ui.h"

extern monitor_data mMonitor;
extern TSettings Settings;

#ifdef PLUS

#define SCR 30
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7796 _panel_instance;
  lgfx::Bus_Parallel8 _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.port = 0;
      cfg.freq_write = 40000000;
      cfg.pin_wr = 47; // pin number connecting WR
      cfg.pin_rd = -1; // pin number connecting RD
      cfg.pin_rs = 0;  // Pin number connecting RS(D/C)
      cfg.pin_d0 = 9;  // pin number connecting D0
      cfg.pin_d1 = 46; // pin number connecting D1
      cfg.pin_d2 = 3;  // pin number connecting D2
      cfg.pin_d3 = 8;  // pin number connecting D3
      cfg.pin_d4 = 18; // pin number connecting D4
      cfg.pin_d5 = 17; // pin number connecting D5
      cfg.pin_d6 = 16; // pin number connecting D6
      cfg.pin_d7 = 15; // pin number connecting D7
      _bus_instance.config(cfg);              // Apply the settings to the bus.
      _panel_instance.setBus(&_bus_instance); // Sets the bus to the panel.
    }
    {                                      // Set display panel control.
      auto cfg = _panel_instance.config(); // Get the structure for display panel settings.
      cfg.pin_cs = -1;   // Pin number to which CS is connected (-1 = disable)
      cfg.pin_rst = 4;   // pin number where RST is connected (-1 = disable)
      cfg.pin_busy = -1; // pin number to which BUSY is connected (-1 = disable)
      // * The following setting values ​​are set to general default values ​​for each panel, and the pin number (-1 = disable) to which BUSY is connected, so please try commenting out any unknown items.
      cfg.memory_width = 320;  // Maximum width supported by driver IC
      cfg.memory_height = 480; // Maximum height supported by driver IC
      cfg.panel_width = 320;   // actual displayable width
      cfg.panel_height = 480;  // actual displayable height
      cfg.offset_x = 0;        // Panel offset in X direction
      cfg.offset_y = 0;        // Panel offset in Y direction
      cfg.offset_rotation = 2;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel_instance.config(cfg);
    }

    {                                      // Set backlight control. (delete if not necessary)
      auto cfg = _light_instance.config(); // Get the structure for backlight configuration.
      cfg.pin_bl = 45;     // pin number to which the backlight is connected
      cfg.invert = false;  // true to invert backlight brightness
      cfg.freq = 44100;    // backlight PWM frequency
      cfg.pwm_channel = 0; // PWM channel number to use
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance); // Sets the backlight to the panel.
    }
    { // Configure settings for touch screen control. (delete if not necessary)
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;   // Minimum X value (raw value) obtained from the touchscreen
      cfg.x_max = 319; // Maximum X value (raw value) obtained from the touchscreen
      cfg.y_min = 0;   // Minimum Y value obtained from touchscreen (raw value)
      cfg.y_max = 479; // Maximum Y value (raw value) obtained from the touchscreen
      cfg.pin_int = 7; // pin number to which INT is connected
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      // For I2C connection
      cfg.i2c_port = 0;    // Select I2C to use (0 or 1)
      cfg.i2c_addr = 0x38; // I2C device address number
      cfg.pin_sda = 6;     // pin number where SDA is connected
      cfg.pin_scl = 5;     // pin number to which SCL is connected
      cfg.freq = 400000;   // set I2C clock
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance); // Set the touchscreen to the panel.
    }
    setPanel(&_panel_instance); // Sets the panel to use.
  }
};

#else

#define SCR 8
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7796 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config(); // Get the structure for bus configuration.
      // SPI bus settings
      cfg.spi_host = VSPI_HOST; // Select the SPI to use ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // * With the ESP-IDF version upgrade, VSPI_HOST and HSPI_HOST descriptions are deprecated, so if an error occurs, use SPI2_HOST and SPI3_HOST instead.
      cfg.spi_mode = 3;                  // Set SPI communication mode (0 ~ 3)
      cfg.freq_write = 27000000;         // SPI clock when sending (up to 80MHz, rounded to 80MHz divided by an integer)
      cfg.freq_read = 6000000;           // SPI clock when receiving
      cfg.spi_3wire = false;             // set to true if receiving on MOSI pin
      cfg.use_lock = true;               // set to true to use transaction lock
      cfg.dma_channel = SPI_DMA_CH_AUTO; // Set the DMA channel to use (0=not use DMA / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=auto setting)
      // * With the ESP-IDF version upgrade, SPI_DMA_CH_AUTO (automatic setting) is recommended for the DMA channel. Specifying 1ch and 2ch is deprecated.
      cfg.pin_sclk = 14; // set SPI SCLK pin number
      cfg.pin_mosi = 13; // Set MOSI pin number for SPI
      cfg.pin_miso = -1; // set SPI MISO pin number (-1 = disable)
      cfg.pin_dc = 21;   // Set SPI D/C pin number (-1 = disable)
      _bus_instance.config(cfg);              // Apply the settings to the bus.
      _panel_instance.setBus(&_bus_instance); // Sets the bus to the panel.
    }
    {                                      // Set display panel control.
      auto cfg = _panel_instance.config(); // Get the structure for display panel settings.
      cfg.pin_cs = 15;   // Pin number to which CS is connected (-1 = disable)
      cfg.pin_rst = 22;  // pin number where RST is connected (-1 = disable)
      cfg.pin_busy = -1; // pin number to which BUSY is connected (-1 = disable)
      // * The following setting values ​​are set to general default values ​​for each panel, and the pin number (-1 = disable) to which BUSY is connected, so please try commenting out any unknown items.
      cfg.memory_width = 320;  // Maximum width supported by driver IC
      cfg.memory_height = 480; // Maximum height supported by driver IC
      cfg.panel_width = 320;   // actual displayable width
      cfg.panel_height = 480;  // actual displayable height
      cfg.offset_x = 0;        // Panel offset in X direction
      cfg.offset_y = 0;        // Panel offset in Y direction
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel_instance.config(cfg);
    }
    {                                      // Set backlight control. (delete if not necessary)
      auto cfg = _light_instance.config(); // Get the structure for backlight configuration.
      cfg.pin_bl = 23;     // pin number to which the backlight is connected
      cfg.invert = false;  // true to invert backlight brightness
      cfg.freq = 44100;    // backlight PWM frequency
      cfg.pwm_channel = 1; // PWM channel number to use
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance); // Sets the backlight to the panel.
    }
    { // Configure settings for touch screen control. (delete if not necessary)
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;    // Minimum X value (raw value) obtained from the touchscreen
      cfg.x_max = 319;  // Maximum X value (raw value) obtained from the touchscreen
      cfg.y_min = 0;    // Minimum Y value obtained from touchscreen (raw value)
      cfg.y_max = 479;  // Maximum Y value (raw value) obtained from the touchscreen
      cfg.pin_int = 39; // pin number to which INT is connected
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      // For I2C connection
      cfg.i2c_port = 1;    // Select I2C to use (0 or 1)
      cfg.i2c_addr = 0x38; // I2C device address number
      cfg.pin_sda = 18;    // pin number where SDA is connected
      cfg.pin_scl = 19;    // pin number to which SCL is connected
      cfg.freq = 400000;   // set I2C clock
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance); // Set the touchscreen to the panel.
    }
    setPanel(&_panel_instance); // Sets the panel to use.
  }
};

#endif

// Create an instance of the prepared class.
LGFX tft;
/* Change to your screen resolution */
static const uint32_t screenWidth = 480;
static const uint32_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_color_t disp_draw_buf[screenWidth * SCR];
static lv_color_t disp_draw_buf2[screenWidth * SCR];

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  if (tft.getStartCount() == 0)
  {
    tft.endWrite();
  }

  tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);

  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  uint16_t touchX, touchY;

  bool touched = tft.getTouch(&touchX, &touchY);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
  }

}

void onBrightnessChange(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_target(e);
  int brightness = (int)lv_slider_get_value(slider);
  tft.setBrightness(brightness);
}

void wt32Display_Init(void)
{
  Serial.println("M5stack display driver initialized");
  tft.init();
  tft.initDMA();
  tft.startWrite();
  lv_init();

  // Setting display to landscape
  if (tft.width() < tft.height())
    tft.setRotation(tft.getRotation() ^ 1);

  Serial.print("Width: ");
  Serial.print(screenWidth);
  Serial.print("\tHeight: ");
  Serial.println(screenHeight);

  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    Serial.print("Display buffer size: ");
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, disp_draw_buf2, screenWidth * SCR);
    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    lv_timer_handler();
  }  
}

void wt32Display_AlternateScreenState(void)
{
}

void wt32Display_AlternateRotation(void)
{
}

static unsigned long ulTime = millis() - 100000;

void wt32Display_NoScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print hashrate to serial
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
  //Serial.printf(">>> Temperature: %s\n", data.temp.c_str());

  lv_label_set_text(ui_lblhashrate, data.currentHashRate.c_str());
  lv_bar_set_value(ui_barhashrate, data.currentHashRate.toInt(), LV_ANIM_ON);
  lv_label_set_text(ui_lblvalid, data.valids.c_str());
  lv_label_set_text(ui_lbltemplates, data.templates.c_str());
  lv_label_set_text(ui_lbltotalhashrate, data.totalKHashes.c_str());
  lv_label_set_text(ui_lblbestdiff, data.bestDiff.c_str());
  lv_label_set_text(ui_lblshares32, data.completedShares.c_str());
  lv_label_set_text(ui_lblclock, data.timeMining.c_str());
  lv_label_set_text(ui_lbltemperature, data.temp.c_str());

  lv_label_set_text(ui_lblclock2, data.currentTime.c_str());

  lv_label_set_text(ui_lblIp, WiFi.localIP().toString().c_str());
  lv_label_set_text(ui_lblAddress, String(Settings.BtcWallet).c_str());

  if(millis() - ulTime > 1000 * 60) {
    ulTime = millis();
  
    coin_data cdata = getCoinData(mElapsed);

    lv_label_set_text(ui_lblPrice, cdata.btcPrice.c_str());
    lv_label_set_text(ui_lblGlobalHashrate, cdata.globalHashRate.c_str());
    lv_label_set_text(ui_lblDifficulty, cdata.netwrokDifficulty.c_str());
    lv_bar_set_value(ui_barhalving, cdata.progressPercent, LV_ANIM_ON);
    lv_label_set_text(ui_lblHeight2, cdata.blockHeight.c_str());

    pool_data pdata = getPoolData();

    lv_label_set_text(ui_lblWorkers, String(pdata.workersCount).c_str());
    lv_label_set_text(ui_lblMaxDifficulty, pdata.bestDifficulty.c_str());
    lv_label_set_text(ui_lblTotHashrate, pdata.workersHash.c_str());
  }
}

void wt32Display_LoadingScreen(void)
{
  Serial.println("Initializing...");
  Serial.print("Firmware Version: ");
  Serial.println(AUTO_VERSION);
  lv_label_set_text(ui_lblssid, "HanSoloAP");
  lv_label_set_text(ui_lblpassword, "MineYourCoins");
  lv_label_set_text(ui_lblversion, AUTO_VERSION);
  lv_label_set_text(ui_lblversion2, AUTO_VERSION);

  lv_label_set_text(ui_lblPool, (String(Settings.PoolAddress)+":"+String(Settings.PoolPort)).c_str());

  _ui_screen_change(&ui_HomeScreen, LV_SCR_LOAD_ANIM_FADE_ON, 2000, 0, &ui_HomeScreen_screen_init);
}

void wt32Display_SetupScreen(void)
{
  Serial.println("Setup...");
}

void wt32Display_DoLedStuff(unsigned long frame)
{
  // we will use led function to update lvgl
  lv_timer_handler();
}

void wt32Display_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction wt32DisplayCyclicScreens[] = {wt32Display_NoScreen};

DisplayDriver wt32DisplayDriver = {
    wt32Display_Init,
    wt32Display_AlternateScreenState,
    wt32Display_AlternateRotation,
    wt32Display_LoadingScreen,
    wt32Display_SetupScreen,
    wt32DisplayCyclicScreens,
    wt32Display_AnimateCurrentScreen,
    wt32Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(wt32DisplayCyclicScreens),
    0,
    0,
    0,
};

// LVGL Events
#include "wManager.h"

void reset_configuration_event(lv_event_t * e){
  reset_configuration();
}

#endif
