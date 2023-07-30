

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "jadeSHA256.h"
#include "customSHA256.h"
#include "nerdSHA256.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include <wolfssl/wolfcrypt/sha256.h>


/********* INIT *****/
void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(0);
  delay(100);

  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  //disableCore0WDT();

  
}

void loop() {

  //Prepare Premining data
    delay(3000);
    uint8_t blockheader[80] = {0};

    for(int i=0; i<80; i++){
        if(i<10) blockheader[i]=0;
        else blockheader[i]=0xFF;
    }
    /* blockheader: 0000000000000000000011111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
       1rstSHA: 2c6b82fa0260c2a3aca4e22444f3133a07990e5d0bb4c0faebef321027af214e
       2ndSHA:  8063482c768e9a922566a895cbc5248ef29f8c0d5a65cc40c64fb74a64ec0a26

       SHA256 from online resources: 
       blockheader: 00000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
       1rstSHA: b36b04e42ed7ffc47f300d8b4f96fef9c987bacf0df40793ec8b194afad3cfbe
       2ndSHA:  7201f38ecedb9b03df0101b407d5c232e62aa76c885d47055fa0f7bd1aa168ec
    */

    Serial.println("");Serial.println("");
    Serial.println("BlockHeader on test: ");
    for (int i = 0; i < 80; i++)
            Serial.printf("%02x", blockheader[i]);
    Serial.println("SHA256 from online resources: ");
    Serial.println("b36b04e42ed7ffc47f300d8b4f96fef9c987bacf0df40793ec8b194afad3cfbe");
    Serial.println("Double SHA256 from online resources:");
    Serial.println("7201f38ecedb9b03df0101b407d5c232e62aa76c885d47055fa0f7bd1aa168ec");
    Serial.println("----------------------------------------------------------------");
    //Test custom SHA
    uint8_t hash[32];
    uint8_t dhash[32];
    uint32_t startT = micros();
    calc_sha_256(hash, blockheader, 80);
    calc_sha_256(dhash, hash, 32);
    uint32_t expired = micros() - startT;
    Serial.println("Custom double SHA [" + String(expired) + "us]:");
    for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", dhash[i]);
    Serial.println("");
    
    //Test WOLF
    Sha256 midstate;
    Sha256 sha256;
    uint8_t hash2[32];
    wc_InitSha256(&midstate);
    wc_Sha256Update(&midstate, blockheader, 64);  
    Serial.print("Wolf midstate: ");
    for (size_t i = 0; i < 8; i++)
            Serial.printf("%02x", midstate.digest[i]);
    Serial.println(""); 
    
    // Mining starts here
    //Primer sha
    startT = micros();
    wc_Sha256Copy(&midstate, &sha256);
    wc_Sha256Update(&sha256, blockheader+64, 16);
    wc_Sha256Final(&sha256, hash2);
    // Segundo SHA-256
    wc_Sha256Update(&sha256, hash2, 32);
    wc_Sha256Final(&sha256, hash2);
    expired = micros() - startT;
    Serial.println("Wolf using midstate  & double SHA[" + String(expired) + "us]:");
    for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash2[i]);
    Serial.println("");
    
    
    //Test mbed
    mbedtls_sha256_context midstate3;
    unsigned char hash3[32];
    mbedtls_sha256_context ctx;
    //Calcular midstate
    mbedtls_sha256_init(&midstate3); 
    mbedtls_sha256_starts_ret(&midstate3, 0);
    mbedtls_sha256_update_ret(&midstate3, blockheader, 64);
    Serial.println("Mbed midstate:");
    for (size_t i = 0; i < 8; i++)
            Serial.printf("%02x", midstate3.state[i]);
    Serial.println(""); 
     for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", midstate3.buffer[i]);
    Serial.println(""); 
    
    // Mining starts here
    // Primer SHA-256
    startT = micros();
    mbedtls_sha256_clone(&ctx, &midstate3); //Clonamos el contexto anterior para continuar el SHA desde allí
    mbedtls_sha256_update_ret(&ctx, blockheader+64, 16);
    mbedtls_sha256_finish_ret(&ctx, hash3);

    // Segundo SHA-256
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, hash3, 32);
    mbedtls_sha256_finish_ret(&ctx, hash3);
    expired = micros() - startT;
    Serial.println("Mbed double SHA[" + String(expired) + "us]:");
    for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash3[i]);
    Serial.println("");   

    
    //Test Jade SHA
    _sha256_context midstate_cached = { 0 };
    memcpy(midstate_cached.buffer, blockheader, 64);
    calc_midstate(blockheader, &midstate_cached);
    Serial.println("Jade midstate:");
    for (size_t i = 0; i < 8; i++)
            Serial.printf("%02x", midstate_cached.state[i]);
    Serial.println(""); 
    for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", midstate_cached.buffer[i]);
    Serial.println(""); 
    *((uint32_t*)&midstate_cached.buffer[12]) = 0xFFFFFFFF;//nonce;
    // Mining starts here
    startT = micros();
    make_double_sha(&midstate_cached);
    expired = micros() - startT;
    Serial.println("Jade double SHA ["+ String(expired) + "us]:");
    for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", midstate_cached.buffer[i]);
    Serial.println("");

     //Test nerdSHA
    nerd_sha256 nerdMidstate;
    uint8_t nerdHash[32];
    nerd_midstate(&nerdMidstate, blockheader, 64);
    Serial.print("Nerd midstate: ");
    for (size_t i = 0; i < 8; i++)
            Serial.printf("%02x", nerdMidstate.digest[i]);
    Serial.println(""); 
    
    //Mining starts here
    startT = micros();
    nerd_double_sha(&nerdMidstate, blockheader+64,nerdHash);
    expired = micros() - startT;
    Serial.println("Nerd double SHA[" + String(expired) + "us]:");
    for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", nerdHash[i]);
    Serial.println("");
    
}       