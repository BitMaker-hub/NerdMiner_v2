# NerdMiner_v2 – Port auf Waveshare ESP32-C6-LCD-1.47

## Ziel
NerdMiner_v2 auf das Waveshare ESP32-C6-LCD-1.47 Board porten.

## Hardware-Fakten (verbindlich, nicht raten)

| Eigenschaft | Wert |
|---|---|
| Chip | ESP32-C6, Single-Core RISC-V, 160 MHz, 4 MB Flash, kein PSRAM |
| Display | ST7789, 172×320 Pixel, SPI, Column-Offset 34 px |
| MOSI | GPIO6 |
| SCLK | GPIO7 |
| LCD_CS | GPIO14 |
| LCD_DC | GPIO15 |
| LCD_RST | GPIO21 |
| Backlight | GPIO22 (aktiv HIGH) |
| RGB-LED (WS2812) | GPIO8 |
| BOOT-Button | GPIO9 |

Referenz: https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47

## Technische Randbedingungen

- **Arduino-Core**: ESP32-C6 erfordert Arduino-Core 3.x → pioarduino-Plattform verwenden (nicht offizielle `espressif32`).
  URL: https://github.com/pioarduino/platform-espressif32/releases
- **TFT_eSPI**: Version 2.5.43 ist inkompatibel mit Core 3.x / ESP32-C6.
  Bekannter Fix: TFT_eSPI Issue #3438 (mboehmerm) / Fork: https://github.com/AndroidCrypto/TFT_eSPI
- **Mining-Vorlage**: Existierendes PlatformIO-Env `ESP32-C3-super-mini` als Vorlage für Single-Core-Mining.
- **Display-Treiber-Vorlage**: TTGO-T-Display-Treiber als Vorlage für ST7789-SPI-Konfiguration.
- **Build-Kommando**: `pio run -e ESP32-C6-LCD-147`

## Arbeitsregeln

- Nach jeder Änderung selbst kompilieren (`pio run -e ESP32-C6-LCD-147`) und Fehler eigenständig fixen, bevor "fertig" gemeldet wird.
- Keine Hardware-Pins raten – ausschließlich obige Tabelle verwenden.
- Keine PSRAM-Nutzung (nicht vorhanden).
- Single-Core-Architektur beachten (kein `xTaskCreatePinnedToCore` auf Core 1).
