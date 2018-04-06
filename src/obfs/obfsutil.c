#include <stdint.h>
#include <time.h>

#include "obfsutil.h"
#include "encrypt.h"
#include <mbedtls/entropy_poll.h>

// Fast PRNG based on SipHash and PCG32

uint64_t siphash(const uint8_t *src, unsigned long src_sz, uint8_t key[16]);

int fast_rand_seed(uint8_t *output, int len, uint64_t *seed) {
    static uint64_t s[2] = {0};
    static int inited = 0;
    if (!inited) {
        size_t olen = 0;
        mbedtls_platform_entropy_poll(&olen, (uint8_t *)s, 16, &olen);
        if (olen == 0) {
            uint32_t seed = (uint32_t)time(NULL);
            s[0] = seed | 0x100000000L;
            s[1] = ((uint64_t)seed << 32) | 0x1;
        }
        inited = 1;
    }

    if (seed) {
        uint64_t buf[3] = {s[0], s[1], *seed};
        s[0] = siphash((uint8_t *)buf, sizeof(buf), (uint8_t *)s);
        s[1] = siphash((uint8_t *)buf, sizeof(buf), (uint8_t *)s);
    }

    while (len > 0) {
        uint32_t rd;
        const int blen = min(len, sizeof(rd));
        uint64_t *state = &s[0];
        uint64_t *inc = &s[1];

        // *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
        // Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
        uint64_t oldstate = *state;
        // Advance internal state
        *state = oldstate * 6364136223846793005ULL + (*inc|1);
        // Calculate output function (XSH RR), uses old state for max ILP
        uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
        uint32_t rot = oldstate >> 59u;
        rd = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

        memcpy(output, (uint8_t *)&rd, blen);
        output += blen;
        len    -= blen;
    }
    return 1;
}

#ifdef PRNG_TEST

#include <TestU01.h>

uint32_t mygen() {
    uint32_t ret = 0;
    fast_rand_seed((uint8_t *)&ret, 4, NULL);
    return ret;
}

int main() {
    // Create TestU01 PRNG object for our generator
    unif01_Gen* gen = unif01_CreateExternGenBits("SipHash", mygen);

    // Run the tests.
#ifdef PRNG_LARGE
    bbattery_BigCrush(gen);
#elif defined(PRNG_BIG)
    bbattery_Crush(gen);
#else
    bbattery_SmallCrush(gen);
#endif

    // Clean up.
    unif01_DeleteExternGenBits(gen);

    return 0;
}

#else

int get_head_size(char *plaindata, int size, int def_size) {
    if (plaindata == NULL || size < 2)
        return def_size;
    int head_type = plaindata[0] & 0x7;
    if (head_type == 1)
        return 7;
    if (head_type == 4)
        return 19;
    if (head_type == 3)
        return 4 + plaindata[1];
    return def_size;
}

static int shift128plus_init_flag = 0;
static uint64_t shift128plus_s[2] = {0x10000000, 0xFFFFFFFF};

void init_shift128plus(void) {
    if (shift128plus_init_flag == 0) {
        shift128plus_init_flag = 1;
        uint32_t seed = (uint32_t)time(NULL);
        shift128plus_s[0] = seed | 0x100000000L;
        shift128plus_s[1] = ((uint64_t)seed << 32) | 0x1;
    }
}

uint64_t xorshift128plus(void) {
    uint64_t x = shift128plus_s[0];
    uint64_t const y = shift128plus_s[1];
    shift128plus_s[0] = y;
    x ^= x << 23; // a
    x ^= x >> 17; // b
    x ^= y ^ (y >> 26); // c
    shift128plus_s[1] = x;
    return x + y;
}

int ss_md5_hmac(char *auth, char *msg, int msg_len, uint8_t *iv, int enc_iv_len, uint8_t *enc_key, int enc_key_len)
{
    uint8_t auth_key[MAX_IV_LENGTH + MAX_KEY_LENGTH];
    memcpy(auth_key, iv, enc_iv_len);
    memcpy(auth_key + enc_iv_len, enc_key, enc_key_len);
    return ss_md5_hmac_with_key(auth, msg, msg_len, auth_key, enc_iv_len + enc_key_len);
}

int ss_sha1_hmac(char *auth, char *msg, int msg_len, uint8_t *iv, int enc_iv_len, uint8_t *enc_key, int enc_key_len)
{
    uint8_t auth_key[MAX_IV_LENGTH + MAX_KEY_LENGTH];
    memcpy(auth_key, iv, enc_iv_len);
    memcpy(auth_key + enc_iv_len, enc_key, enc_key_len);
    return ss_sha1_hmac_with_key(auth, msg, msg_len, auth_key, enc_iv_len + enc_key_len);
}

void memintcopy_lt(void *mem, uint32_t val) {
    ((uint8_t *)mem)[0] = (uint8_t)(val);
    ((uint8_t *)mem)[1] = (uint8_t)(val >> 8);
    ((uint8_t *)mem)[2] = (uint8_t)(val >> 16);
    ((uint8_t *)mem)[3] = (uint8_t)(val >> 24);
}

#endif

