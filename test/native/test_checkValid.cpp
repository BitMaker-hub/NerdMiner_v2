// ---------------------------------------------------------------------------
//  Unit test for checkValid() — the "did we just mine a block?" predicate.
//
//  Why this test exists: checkValid() decides whether the firmware reports
//  "CONGRATULATIONS! Valid block found" (mining.cpp, STRATUM_SUCCESS branch).
//  Three defects made it unreliable in both directions:
//    1. memcpy(diff_target, &target, 32)  — copied the address of the
//       parameter instead of the 32 target bytes.
//    2. for(uint8_t i=31; i>=0; i--)      — unsigned counter, `i >= 0` is
//       always true; after 0 it wrapped to 255 and read out of bounds.
//    3. the loop only broke on `>`, never accepting early on `<`, so a hash
//       strictly below the target could still be rejected by a lower byte.
//
//  The oracle below is an independent big-endian comparison written from the
//  definition ("hash <= target, most significant byte first"), not from the
//  implementation, so it cannot inherit the same mistakes.
// ---------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <random>

extern bool checkValid(unsigned char* hash, unsigned char* target);
extern void reverse_bytes(uint8_t* data, size_t len);

static int fails = 0;
static int checks = 0;

// Oracle: target arrives big-endian (as mMiner.bytearray_target is built);
// hash is little-endian (as nerd_sha256d_* stores it). Compare as 256-bit
// numbers: hash <= target.
static bool oracle(const unsigned char* hash_le, const unsigned char* target_be)
{
    for (int i = 0; i < 32; ++i) {
        unsigned char h = hash_le[31 - i];   // most significant first
        unsigned char t = target_be[i];
        if (h > t) return false;
        if (h < t) return true;
    }
    return true;                              // exactly equal -> valid
}

static void expect(const char* what, const unsigned char* hash_le,
                   const unsigned char* target_be)
{
    unsigned char h[32], t[32];
    memcpy(h, hash_le, 32);
    memcpy(t, target_be, 32);
    bool got = checkValid(h, t);
    bool want = oracle(hash_le, target_be);
    ++checks;
    if (got != want) {
        ++fails;
        printf("  FAIL %-46s got=%d want=%d\n", what, (int)got, (int)want);
        printf("       hash(le) "); for (int i = 31; i >= 0; --i) printf("%02x", hash_le[i]);
        printf("\n       target   "); for (int i = 0; i < 32; ++i) printf("%02x", target_be[i]);
        printf("\n");
    }
    // The implementation must not modify its inputs.
    if (memcmp(h, hash_le, 32) != 0) { ++fails; printf("  FAIL %s: hash degisti\n", what); }
}

int main()
{
    printf("checkValid() birim testi\n");

    // Real nbits 0x1701e5ac target, big-endian (bytes 9..11 = 01 e5 ac).
    unsigned char net[32] = {0};
    net[9] = 0x01; net[10] = 0xe5; net[11] = 0xac;

    unsigned char zero[32] = {0};
    unsigned char ones[32];  memset(ones, 0xff, 32);

    // --- sanity: reverse_bytes is its own inverse ---
    {
        unsigned char a[32], b[32];
        for (int i = 0; i < 32; ++i) a[i] = (unsigned char)(i * 7 + 1);
        memcpy(b, a, 32);
        reverse_bytes(b, 32); reverse_bytes(b, 32);
        ++checks;
        if (memcmp(a, b, 32) != 0) { ++fails; printf("  FAIL reverse_bytes tersinir degil\n"); }
    }

    // --- edge cases ---
    expect("hash 0 (en kucuk) vs ag hedefi", zero, net);
    expect("hash 0xff.. (en buyuk) vs ag hedefi", ones, net);
    expect("hash == target (sinir, gecerli olmali)", /*placeholder*/ zero, zero);
    expect("hedef 0: sadece hash 0 gecerli", ones, zero);
    expect("hedef max: her hash gecerli", ones, ones);

    // hash exactly equal to the network target, expressed little-endian
    {
        unsigned char h[32];
        for (int i = 0; i < 32; ++i) h[i] = net[31 - i];
        expect("hash tam hedefe esit", h, net);
    }
    // one below / one above the target in the most significant differing byte
    {
        unsigned char h[32];
        for (int i = 0; i < 32; ++i) h[i] = net[31 - i];
        h[31 - 9] = 0x00;                      // high byte smaller -> valid
        expect("hash hedefin ustunde bir bayt kucuk", h, net);
        for (int i = 0; i < 32; ++i) h[i] = net[31 - i];
        h[31 - 9] = 0x02;                      // high byte bigger -> invalid
        expect("hash hedefin ustunde bir bayt buyuk", h, net);
    }
    // DEFECT 3 regression: below target high up, but a LOW byte exceeds the
    // target's low byte. Old code wrongly returned false here.
    {
        unsigned char h[32];
        memset(h, 0, 32);
        h[31 - 9] = 0x00; h[31 - 10] = 0x00; h[31 - 11] = 0x00;
        h[31 - 12] = 0xff;                     // far below target, big low byte
        expect("yuksek baytta kucuk + alt baytta buyuk (kusur 3)", h, net);
    }
    // DEFECT 2 regression: all-zero hash and all-zero target exercise the
    // full 32-iteration path; the old unsigned loop read index 255 here.
    expect("tam 32 tur (kusur 2: dizi disi okuma)", zero, zero);

    // --- randomised, against the oracle ---
    std::mt19937_64 rng(20261105);
    for (int t = 0; t < 200000; ++t) {
        unsigned char h[32], g[32];
        for (int i = 0; i < 32; ++i) h[i] = (unsigned char)(rng() & 0xff);
        memcpy(g, net, 32);
        // half the cases: make the hash plausibly close to the target so the
        // comparison actually has to walk several bytes
        if (t & 1) {
            memset(h + 28, 0, 4);
            h[31 - 9] = (unsigned char)(rng() % 3);
        }
        if ((t % 7) == 0) { for (int i = 0; i < 32; ++i) g[i] = (unsigned char)(rng() & 0xff); }
        expect("rastgele", h, g);
        if (fails > 5) break;
    }

    printf("%s  %d kontrol, %d basarisiz\n", fails ? "SONUC: BASARISIZ" : "SONUC: TUMU GECTI",
           checks, fails);
    return fails ? 1 : 0;
}
