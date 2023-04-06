# NerdSoloMiner
The NerdSoloMiner v2

Original project https://github.com/valerio-vaccaro/HAN

![image](https://raw.githubusercontent.com/BitMaker-hub/NerdMiner_v2/master/images/NerdMinerv2.jpg)

## Requirements
- TTGO T-Display S3
- 3D BOX

## Description
ESP32 implementing Stratum protocol to mine on solo pool. Pool can be changed but originally works with ckpool.

This project is using ESP32-S3, uses WifiManager to modify miner settings and save them to SPIFF. 

This miner is multicore and multithreads, each thread mine a different block template. After 1,000,000 trials the block in refreshed in order to avoid mining on old template.

## TUTORIAL
Create your own miner using the online tool ESPtool and the binary files that you will find in the bin folder.
If you want you can compile the entire project using Arduino, PlatformIO or Expressif IDF.

1. Get a TTGO T-display S3
1. Download this repository
1. Go to ESPtool online: https://espressif.github.io/esptool-js/
1. Load the firmware with the binaries from the bin folder.

Complete tutorial on YouTube:
 
[![Ver video aquí](https://img.youtube.com/vi/POUT2R_opDs/0.jpg)](https://youtu.be/POUT2R_opDs)

## DEVELOPMENT
You can use platformio to develop this project.
