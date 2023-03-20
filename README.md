# NerdSoloMiner
A NerdSoloMiner using > Han miner

Original project https://github.com/valerio-vaccaro/HAN

![image](https://github.com/cryptopasivo/NerdMiner_v2/blob/master/images/NerdMinerv2.jpg)

## Requirements
- TTGO T-Display S3
- 3D BOX
- Program from Expressif webtool

## Description
ESP32 implementing Stratum protocol to mine on solo pool. Pool can be changed but originally works with ckpool.

This miner is multicore and multithreads, each thread mine a different block template. After 1,000,000 trials the block in refreshed in order to avoid mining on old template.

## HW Schematic
Connect your ESP32 following this image.

You will find all STL files to build the box.

![image]https://github.com/cryptopasivo/ESP32_NerdMiner/blob/master/ASIC_BOX/ASIC_BOX.png)

You can add a 5V fan just for fun and to make your miner pretty.


## Selecting pool and payment address
All configurations are saved in the file `config.h`.

Wifi can be set using `WIFI_SSID` and `WIFI_PASSWORD` constants.

`THREADS` defines the number of concurrent threads used, every thread will work on a different template.

Every thread will use a progressive nonce from 0 to `MAX_NONCE`, when nonce will be equal to `MAX_NONCE` a new template will be downloaded and nonce will be reset to 0.

Funds will go to the address writte in `ADDRESS`.

`POOL_URL` and `POOL_PORT` are used for select the solo pool.
