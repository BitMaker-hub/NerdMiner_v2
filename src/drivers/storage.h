#ifndef _STORAGE_H_
#define _STORAGE_H_

class TSettings
{
public:
	char WifiSSID[80]{ "sA51" };
	char WifiPW[80]{ "0000" };
	char PoolAddress[80]{ "public-pool.io" };
	char BtcWallet[80]{ "yourBtcAddress" };
	uint32_t PoolPort{ 21496 };
	uint32_t Timezone{ 2 };
	bool holdsData{ false };
};

#endif // _STORAGE_H_