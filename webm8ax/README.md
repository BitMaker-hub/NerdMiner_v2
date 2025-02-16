# ESP-Web-Tools-Tutorial
A guide for setting up flashing your ESP projects directly from your browser

[ESP-Web-Tools](https://esphome.github.io/esp-web-tools/) allows your to flash your ESP directly from your browser using [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API). This repo will provide the instructions needed to get this up and running.

You should note that the Web Serial API has some requirements:

- [Limited browser support](https://developer.mozilla.org/en-US/docs/Web/API/Serial) - Basically just Chrome and Edge.
- Website requires https - For this example we will use github pages to host the site, which supports https and is free.

You should also note that the sketchs that are uploaded will have to be generic ones, so can't have configurations like WiFi details so please use something like [WiFiManager](https://github.com/tzapu/WiFiManager) to solve this, or you can use [Improv Wifi](https://www.improv-wifi.com/)


## Help support what I do!

I have put a lot of effort into creating Arduino libraries that I hope people can make use of. [If you enjoy my work, please consider becoming a Github sponsor!](https://github.com/sponsors/witnessmenow/)


## Setting Up Github Pages

Github Pages is a feature of github that allows you to create a static website for your projects. It's free and quite easy to setup. This isn't a full guide for Github pages, but just shows how to configure it for this use case. When you create a github pages site for your project it will live under *USER_NAME*.github.io/*REPO_NAME*, so for example the URL for this repo would be `witnessmenow.github.io/ESP-Web-Tools-Tutorial`

1. Click `Settings` on the repo you want to add the website to and scroll down to the `GitHub Pages` section
2. Under `Source`, change the drop down to your `main` branch (could be `master` if it's an older repo) 

And that's it! when you visit *USER_NAME*.github.io/*REPO_NAME* you should see your projects readme converted to a webpage.

## Creating a flashing page and folder structure

**Note:** I would test this out with a new dummy repo before adding it to your project.

1. In your Github repo, create a new file on the root, `flash.html`
2. Populate the file with the following (you can find more details on the [ESP-Web-tools](https://esphome.github.io/esp-web-tools/) page):
```
<!DOCTYPE html>
<html>
<body>
    <h1>ESP-Web-Tool-Test</h1>
    <script type="module" src="https://unpkg.com/esp-web-tools@3.4.2/dist/web/install-button.js?module"></script>
    <esp-web-install-button manifest="test/manifest.json"></esp-web-install-button>
</body>
</html>
```
3. In your repo, create a `test` folder
4. In the `test` folder, create a `manifest.json` file.
5. This is the example manifest.json that ESP-Web-Tools provide, you can add that for now:
```
{
  "name": "ESPHome",
  "builds": [
    {
      "chipFamily": "ESP32",
      "improv": true,
      "parts": [
        { "path": "bootloader.bin", "offset": 4096 },
        { "path": "partitions.bin", "offset": 32768 },
        { "path": "ota.bin", "offset": 57344 },
        { "path": "firmware.bin", "offset": 65536 }
      ]
    },
    {
      "chipFamily": "ESP8266",
      "parts": [
        { "path": "esp8266.bin", "offset": 0 }
      ]
    }
  ]
}
```
6. You can update the `name` field for your project.
7. From the example you can see it supports different arcitechures, it will automatically select the appropriate one based on the device thats plugged in. For now remove the entry for the chip you will not be using. (make sure to remove the `,` too!)
8. If you are using the ESP32, set the `improv` field to `false`

## Getting the image(s) to flash

This was actually the part I found the hardest, just because I was completely unfamilar what happens once you cick "Upload"!

### For Arduino IDE

NOTE: Only the bin files under the temporary Arduino folder will change when you update the sketch, you can re-use the other ones.

1. In the Arduino IDE, click **File** -> **Preferences**
2. Under **Show Verbose Ouput**, check the **Upload** option.
3. Program your board with the project you want to create the flasher for.
4. In the logs scroll up to top , there should be a line with **esptool**, mine looked like this:
```
/Users/brian/Library/Arduino15/packages/esp32/tools/esptool_py/3.0.0/esptool --chip esp32 
--port /dev/cu.SLAB_USBtoUART --baud 921600 --before default_reset --after hard_reset write_flash 
-z --flash_mode dio --flash_freq 80m --flash_size detect 
0xe000 /Users/brian/Library/Arduino15/packages/esp32/hardware/esp32/1.0.6/tools/partitions/boot_app0.bin 
0x1000 /Users/brian/Library/Arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/bin/bootloader_qio_80m.bin 
0x10000 /var/folders/tn/lk3_b69x7kg06xgj6tc4x2000000gn/T/arduino_build_379859/BlinkREgular.ino.bin 
0x8000 /var/folders/tn/lk3_b69x7kg06xgj6tc4x2000000gn/T/arduino_build_379859/BlinkREgular.ino.partitions.bin 

```
5. This particular example contains 4 bin files and their offsets. The offset comes before the bin address, so the 4 above looks like this extracted out

| Offset(hex) | Bin Location |
| ----------- | ------------ |
|0xe000|/Users/brian/Library/Arduino15/packages/esp32/hardware/esp32/1.0.6/tools/partitions/boot_app0.bin|
|0x1000|/Users/brian/Library/Arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/bin/bootloader_qio_80m.bin|
|0x10000|/var/folders/tn/lk3_b69x7kg06xgj6tc4x2000000gn/T/arduino_build_379859/BlinkREgular.ino.bin|
|0x8000|/var/folders/tn/lk3_b69x7kg06xgj6tc4x2000000gn/T/arduino_build_379859/BlinkREgular.ino.partitions.bin|

6. The Manifest file expexts the offset in decimal, so [convert the hex to decimal](https://www.rapidtables.com/convert/number/hex-to-decimal.html)

| Offset(hex) | Offset(decimal) | Bin Location |
| ----------- | --------------- | ------------ |
|0xe000|57344|/Users/brian/Library/Arduino15/packages/esp32/hardware/esp32/1.0.6/tools/partitions/boot_app0.bin|
|0x1000|4096|/Users/brian/Library/Arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/bin/bootloader_qio_80m.bin|
|0x10000|65536|/var/folders/tn/lk3_b69x7kg06xgj6tc4x2000000gn/T/arduino_build_379859/BlinkREgular.ino.bin|
|0x8000|32768|/var/folders/tn/lk3_b69x7kg06xgj6tc4x2000000gn/T/arduino_build_379859/BlinkREgular.ino.partitions.bin|

### For Platformio

[This is untested by me, but it looks right](https://community.platformio.org/t/export-of-binary-firmware-files-for-esp32-download-tool/9253/2)

## Updating the manifest and adding files

1. Copy each of the bin files from the previous step to the `test` folder we created earlier (so it should be beside the `manifest.json`)
2. Add an entry in `parts` for each bin file using the file name as the `path` and the deimal offset for `offset`. The above example would look like this:
```
{
  "name": "WebToolTest",
  "builds": [
    {
      "chipFamily": "ESP32",
      "improv": false,
      "parts": [
        { "path": "bootloader_qio_80m.bin", "offset": 4096 },
        { "path": "BlinkREgular.ino.partitions.bin", "offset": 32768 },
        { "path": "boot_app0.bin", "offset": 57344 },
        { "path": "BlinkREgular.bin", "offset": 65536 }
      ]
    }
  ]
}
```
3. You can now commit your changes and push to you repo.

**NOTE:** There is an arguement that it's not good practise to add binaries into source control like this, but it does not seem like its possible to get the binary from the releases section due to CORS, so I think this will be fine.

## Testing

1. Using Chrome or Edge, visit *USER_NAME*.github.io/*REPO_NAME*/flash.html
2. Plug your board into your computer
3. Click the `Install` button
4. You should be prompted to select a USB port, pick your device. (This step requires drivers, presumably you already have them if you are here though!)
5. It should now start flashing, when its finished you should have a flashed board!
