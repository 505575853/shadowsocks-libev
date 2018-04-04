/* <MIT License>
 Copyright (c) 2013  Marek Majkowski <marek@popcount.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 </MIT License>

 Original location:
    https://github.com/majek/csiphash/

 Solution inspired by code from:
    Samuel Neves (supercop/crypto_auth/siphash24/little)
    djb (supercop/crypto_auth/siphash24/little2)
    Jean-Philippe Aumasson (https://131002.net/siphash/siphash24.c)
*/

#include <stdint.h>
#include <string.h>

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define _le64toh(x) ((uint64_t)(x))
#elif defined(_WIN32)
/* Windows is always little endian, unless you're on xbox360
   http://msdn.microsoft.com/en-us/library/b0084kay(v=vs.80).aspx */
#  define _le64toh(x) ((uint64_t)(x))
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define _le64toh(x) OSSwapLittleToHostInt64(x)
#else

/* See: http://sourceforge.net/p/predef/wiki/Endianness/ */
#  if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#    include <sys/endian.h>
#  else
#    include <endian.h>
#  endif
#  if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
    __BYTE_ORDER == __LITTLE_ENDIAN
#    define _le64toh(x) ((uint64_t)(x))
#  else
#    define _le64toh(x) le64toh(x)
#  endif

#endif


#define ROTATE(x, b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define HALF_ROUND(a,b,c,d,s,t)         \
    a += b; c += d;             \
    b = ROTATE(b, s) ^ a;           \
    d = ROTATE(d, t) ^ c;           \
    a = ROTATE(a, 32);

#define DOUBLE_ROUND(v0,v1,v2,v3)       \
    HALF_ROUND(v0,v1,v2,v3,13,16);      \
    HALF_ROUND(v2,v1,v0,v3,17,21);


static uint64_t siphash(const uint8_t *src, unsigned long src_sz, uint8_t key[16]) {
    const uint64_t *_key = (uint64_t *)key;
    uint64_t k0 = _le64toh(_key[0]);
    uint64_t k1 = _le64toh(_key[1]);
    uint64_t b = (uint64_t)src_sz << 56;
    const uint64_t *in = (uint64_t*)src;

    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;

    while (src_sz >= 8) {
        uint64_t mi = _le64toh(*in);
        in += 1; src_sz -= 8;
        v3 ^= mi;
        DOUBLE_ROUND(v0,v1,v2,v3);
        v0 ^= mi;
    }

    uint64_t t = 0; uint8_t *pt = (uint8_t *)&t; uint8_t *m = (uint8_t *)in;
    switch (src_sz) {
    case 7: pt[6] = m[6];
    case 6: pt[5] = m[5];
    case 5: pt[4] = m[4];
    case 4: *((uint32_t*)&pt[0]) = *((uint32_t*)&m[0]); break;
    case 3: pt[2] = m[2];
    case 2: pt[1] = m[1];
    case 1: pt[0] = m[0];
    }
    b |= _le64toh(t);

    v3 ^= b;
    DOUBLE_ROUND(v0,v1,v2,v3);
    v0 ^= b; v2 ^= 0xff;
    DOUBLE_ROUND(v0,v1,v2,v3);
    DOUBLE_ROUND(v0,v1,v2,v3);
    return (v0 ^ v1) ^ (v2 ^ v3);
}

#undef ROTATE
#undef HALF_ROUND
#undef DOUBLE_ROUND

static int ss_fast_hash_with_key(char *auth, char *msg, int msg_len, uint8_t *auth_key, int key_len)
{
    union {
        uint8_t bytes[8];
        uint64_t num;
    } hash; // 64-bit output
    uint8_t key[16] = {0}; // 128-bit key

    if (key_len != 16) {
        uint8_t* in_key = auth_key;
        if (key_len < 16) {
            memcpy(key, auth_key, key_len); // padding with zero
            in_key = key;
        }
        hash.num = siphash(auth_key, key_len, in_key);
        memcpy(key, hash.bytes, 8);
        memcpy(key + 8, hash.bytes, 8);
    }

    hash.num = siphash((uint8_t *)msg, msg_len, key);
    memcpy(auth, hash.bytes, 8);

    return 0;
}

static int ss_fast_hash_func(char *auth, char *msg, int msg_len)
{
    return ss_fast_hash_with_key(auth, msg, msg_len, (uint8_t *)msg, msg_len);
}

#ifdef SIPHASH_TEST

#include <stdio.h>

void print_hash(uint8_t bytes[8])
{
    for (int i = 0; i < 8; i++) {
        printf("%02x", bytes[i]);
    }
}

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#elif defined(_WIN32)
#include <winsock2.h>
#define htobe64(x) htonll(x)
#endif

#define HASH_TEST(h,k,m,r) do {\
    ss_fast_hash_with_key((char *)h, (char *)m, sizeof(m) - 1, (uint8_t *)k, sizeof(k) - 1); \
    print_hash(h); \
    uint64_t a = htobe64(0x ## r ## ULL); \
    if (memcmp(h, (uint8_t *) &a, 8) == 0) { \
        puts(": OK"); \
    } else { \
        puts(": FAILED"); \
    } \
} while(0);

int main(void)
{
    uint8_t hash[8];
    HASH_TEST(hash, "0123456789ABCDEFG", "a", fc70b99def0b2f5e);
    HASH_TEST(hash, "0123456789ABCDEFG", "b", 391201c9f952f870);
    HASH_TEST(hash, "0123456789AB", "a", 9ab9a95298e6c35e);
    HASH_TEST(hash, "0123456789AB\x00", "a", e7c2c44bcc1a49e7);
    HASH_TEST(hash, "0123456789ABCDEFGHIJ", "a", e64d5548fe10da6c);
    HASH_TEST(hash, "0123456789ABCDEFGHIK", "a", 14e13ccb701005f9);
    return 0;
}

#endif
