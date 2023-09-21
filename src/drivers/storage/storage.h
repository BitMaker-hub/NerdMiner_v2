#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <Arduino.h>

#define DEFAULT_SSID		"NerdMinerAP"
#define DEFAULT_WIFIPW		"MineYourCoins"
#define DEFAULT_POOLURL		"public-pool.io"
#define DEFAULT_WALLETID	"yourBtcAddress"
#define DEFAULT_POOLPORT	21496
#define DEFAULT_TIMEZONE	2
#define DEFAULT_SAVESTATS	false

// JSON config file
#define JSON_CONFIG_FILE "/config.json"
#define JSON_KEY_SSID	"SSID"
#define JSON_KEY_PASW	"PW"
#define JSON_KEY_POOLURL	"PoolUrl"
#define JSON_KEY_WALLETID	"BtcWallet"
#define JSON_KEY_POOLPORT	"PoolPort"
#define JSON_KEY_TIMEZONE	"Timezone"
#define JSON_KEY_STATS2NV	"saveStats"

struct TSettings
{
	String WifiSSID{ DEFAULT_SSID };
	String WifiPW{ DEFAULT_WIFIPW };
	String PoolAddress{ DEFAULT_POOLURL };
	char BtcWallet[80]{ DEFAULT_WALLETID };
	int PoolPort{ DEFAULT_POOLPORT };
	int Timezone{ DEFAULT_TIMEZONE };
	bool saveStats{ DEFAULT_SAVESTATS };
};

#endif // _STORAGE_H_