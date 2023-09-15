#ifndef _STORAGE_H_
#define _STORAGE_H_

#define DEFAULT_SSID		"NMAP"
#define DEFAULT_WIFIPW		"1234567890"
#define DEFAULT_POOLURL		"public-pool.io"
#define DEFAULT_WALLETID	"yourBtcAddress"
#define DEFAULT_POOLPORT	21496
#define DEFAULT_TIMEZONE	2

// JSON config file
#define JSON_CONFIG_FILE "/config.json"
#define JSON_KEY_SSID	"SSID"
#define JSON_KEY_PASW	"PW"
#define JSON_KEY_POOLURL	"PoolUrl"
#define JSON_KEY_WALLETID	"BtcWallet"
#define JSON_KEY_POOLPORT	"PoolPort"
#define JSON_KEY_TIMEZONE	"Timezone"

struct TSettings
{
	char WifiSSID[80]{ DEFAULT_SSID };
	char WifiPW[80]{ DEFAULT_WIFIPW };
	char PoolAddress[80]{ DEFAULT_POOLURL };
	char BtcWallet[80]{ DEFAULT_WALLETID };
	uint32_t PoolPort{ DEFAULT_POOLPORT };
	uint32_t Timezone{ DEFAULT_TIMEZONE };
};

#endif // _STORAGE_H_