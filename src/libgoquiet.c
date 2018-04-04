#include "sds.h"
#include <stdlib.h>
#include <string.h>

#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <mbedtls/entropy.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/entropy_poll.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)
#endif

#ifdef __MINGW32__
#include <windows.h>
#define POW10_7                 10000000

/* Number of 100ns-seconds between the beginning of the Windows epoch
 * (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
 */
#define DELTA_EPOCH_IN_100NS    INT64_C(116444736000000000)
#endif

#include <time.h>

#define SHA256_BYTES 32
#define DEFAULT_TIME_HINT 3600

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define INIT_FIELD(field) char *p ## field = NULL
#define SET_FIELD(opt, st, field) do { \
    if (p ## field == NULL) { \
        p ## field = strstr(opt, #field "="); \
        if (p ## field != NULL) { \
            p ## field += sizeof(#field "=") - 1; \
            (st).field = sdsnew(p ## field); \
        } \
    } \
} while(0)
#define CHK_FIELD(err, field) do { \
    if (p ## field == NULL) { \
        (err) = 1; \
    } \
} while(0)
#define make_sds(len) sdsgrowzero(sdsempty(), len)
#define makeNullBytes(len) make_sds(len)
#define make_bytes(bytes...) sdsnewlen((uint8_t[]){bytes}, (sizeof((uint8_t[]){bytes})))
#define append(a, b) a = sdscatsds((a), (b))
#define appendf(a, b) do { \
    a = sdscatsds((a), (b)); \
    sdsfree(b); \
} while(0)
#define MKBYTE (uint8_t[])

#define PUT_UINT16_BE(b, v) do { \
    (b)[0] = (unsigned char) ((v) >> 8); \
    (b)[1] = (unsigned char) ((v)     ); \
} while(0)

typedef struct _State {
    int Opaque;
    sds Key;
    sds TicketTimeHint;
    int TicketTimeHintInt;
    sds AESKey;
    sds ServerName;
    sds Browser;
} State;

static State sta;

// BtoInt converts a byte slice into int in Big Endian order
// Uint methods from binary package can be used, but they are messy
static int BtoInt(sds b) {
    unsigned int mult = 1;
    unsigned int sum = 0;
    unsigned int length = sdslen(b);
    unsigned int i;
    for (i = 0; i < length; i++) {
        sum += ((unsigned int)b[i]) * (mult << ((length - i - 1) * 8));
    }
    return (int)sum;
}

// Do not use this function, only for failsafe
// RC4 PRNG: http://www.stanford.edu/class/cs140/projects/pintos/pintos.html
static void fallback_rand(void *input, size_t size) {
#define swap_byte(a, b) do { \
    uint8_t t = *(a); \
    *(a) = *(b); \
    *(b) = t; \
} while(0)
    static uint8_t s[256];
    static uint8_t s_i, s_j;
    static int inited = 0;
    uint8_t *buf;

    if (!inited) {
        unsigned int seed = 0;
        uint8_t *seedp = (uint8_t *)&seed;
        int i;
        uint8_t j;

        for (i = 0; i < 256; i++) {
            s[i] = i;
        }
        for (i = j = 0; i < 256; i++)  {
            j += s[i] + seedp[i % sizeof seed];
            swap_byte(s + i, s + j);
        }

        s_i = s_j = 0;
        inited = 1;
    }

    for (buf = input; size-- > 0; buf++) {
        uint8_t s_k;
      
        s_i++;
        s_j += s[s_i];
        swap_byte(s + s_i, s + s_j);

        s_k = s[s_i] + s[s_j];
        *buf = s[s_k];
    }
#undef swap_byte
}

static void random_bytes(uint8_t *output, int len) {
    static mbedtls_entropy_context ec = {};
    static mbedtls_hmac_drbg_context cd_ctx = {};
    static unsigned char rand_initialised = 0;
    if (!rand_initialised) {
        size_t olen;
        uint8_t rand_buffer[8];
        mbedtls_platform_entropy_poll(&olen, rand_buffer, 8, &olen);
        mbedtls_entropy_init(&ec);
        mbedtls_hmac_drbg_init(&cd_ctx);
        if (mbedtls_hmac_drbg_seed(&cd_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1),
                                   mbedtls_entropy_func, &ec,
                                   (const unsigned char *)rand_buffer, 8) != 0) {
            mbedtls_entropy_free(&ec);
            mbedtls_hmac_drbg_free(&cd_ctx);
            fallback_rand(output, len);
            return;
        }
        rand_initialised = 1;
    }
    while (len > 0) {
        const size_t blen = min(len, MBEDTLS_HMAC_DRBG_MAX_REQUEST);
        if (mbedtls_hmac_drbg_random(&cd_ctx, output, blen) != 0) {
            fallback_rand(output, len);
            return;
        }
        output += blen;
        len    -= blen;
    }
}

// CryptoRandBytes generates a byte slice filled with cryptographically secure random bytes
static sds CryptoRandBytes(int len) {
    sds buf = make_sds(len);
    random_bytes((uint8_t *)buf, len);
    return buf;
}

// PsudoRandBytes returns a byte slice filled with psudorandom bytes generated by the seed
static sds PsudoRandBytes(int len, int64_t seed) {
    mbedtls_hmac_drbg_context cd_ctx = {};
    union {
        uint8_t buf[8];
        int64_t num;
    } rand_buffer;
    sds ret = make_sds(len);
    uint8_t *output = (uint8_t *)ret;
    rand_buffer.num = seed;
    mbedtls_hmac_drbg_init(&cd_ctx);
    if (mbedtls_hmac_drbg_seed_buf(&cd_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1),
                                   (const unsigned char *)rand_buffer.buf, 8) != 0) {
        mbedtls_hmac_drbg_free(&cd_ctx);
        fallback_rand(output, len);
        return ret;
    }
    while (len > 0) {
        const size_t blen = min(len, MBEDTLS_HMAC_DRBG_MAX_REQUEST);
        if (mbedtls_hmac_drbg_random(&cd_ctx, output, blen) != 0) {
            fallback_rand(output, len);
            return ret;
        }
        output += blen;
        len    -= blen;
    }
    mbedtls_hmac_drbg_free(&cd_ctx);
    return ret;
}

static int randIntn(int upper) {
    int ret = 0;
    sds bytes = CryptoRandBytes(4);
    union {
        uint8_t buf[4];
        int32_t num;
    } rand;
    memcpy(rand.buf, bytes, 4);
    ret = ((int)rand.num) % upper;
    sdsfree(bytes);
    return ret;
}

static sds DecodeHexString(const char *s) {
    size_t len = strlen(s) / 2;
    sds ret = make_sds(len);
    size_t i, j, k;
    for (i = 0; i < len * 2; i++, s++ ) {
        if (*s >= '0' && *s <= '9')
            j = *s - '0';
        else if( *s >= 'A' && *s <= 'F' )
            j = *s - '7';
        else if( *s >= 'a' && *s <= 'f' )
            j = *s - 'W';
        else
            j = 0;
        k = ((i & 1) != 0) ? j : j << 4;
        ret[i >> 1] = (unsigned char)(ret[i >> 1] | k);
    }
    return ret;
}

static int64_t NowUnixNano() {
    struct timespec now;
    int64_t second;
#if defined(__APPLE__)
    static uint64_t orwl_timestart = 0;
    static double orwl_timebase = 0.0;
    if (!orwl_timestart) {
        mach_timebase_info_data_t tb = { 0 };
        mach_timebase_info(&tb);
        orwl_timebase = tb.numer;
        orwl_timebase /= tb.denom;
        orwl_timestart = mach_absolute_time();
    }
    double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
    now.tv_sec = diff * ORWL_NANO;
    now.tv_nsec = diff - (now.tv_sec * ORWL_GIGA);
#elif defined(__MINGW32__)
    unsigned __int64 t;
    union {
        unsigned __int64 u64;
        FILETIME ft;
    } ct;
    GetSystemTimeAsFileTime(&ct.ft);
    t = ct.u64 - DELTA_EPOCH_IN_100NS;
    now.tv_sec = t / POW10_7;
    now.tv_nsec = ((int) (t % POW10_7)) * 100;
#else
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
        return time(NULL) * 1e9;
    }
#endif
    second = ((int64_t)now.tv_sec) * 1e9 + ((int64_t)now.tv_nsec);
    return second;
}

// SetAESKey calculates the SHA256 of the string key
static void SetAESKey(State* sta) {
    uint8_t hash[SHA256_BYTES];
    mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 
               (uint8_t *)sta->Key, sdslen(sta->Key), (uint8_t *)hash);
    sta->AESKey = sdsnewlen(hash, SHA256_BYTES);
}

// AddRecordLayer adds record layer to data
static sds AddRecordLayer(sds input, uint8_t typ[1], uint8_t ver[2]) {
    sds length = make_sds(2);
    PUT_UINT16_BE(length, (uint16_t)(sdslen(input)));
    sds ver_dup = sdsnewlen(ver, 2);
    sds ret = sdsnewlen(typ, 1);
    appendf(ret, ver_dup);
    appendf(ret, length);
    appendf(ret, input);
    return ret;
}

static sds makeServerName(State *sta) {
    sds serverName = sta->ServerName;
    sds serverNameLength = make_sds(2);
    PUT_UINT16_BE(serverNameLength, (uint16_t)(sdslen(serverName)));
    sds ret = make_bytes(0x00); // host_name
    appendf(ret, serverNameLength);
    append(ret, serverName);
    sds serverNameListLength = make_sds(2);
    PUT_UINT16_BE(serverNameListLength, (uint16_t)(sdslen(ret)));
    appendf(serverNameListLength, ret);
    return serverNameListLength;
}

static sds makeSessionTicket(State *sta) {
    int64_t seed = (int64_t)(sta->Opaque + BtoInt(sta->AESKey) + ((int)time(NULL))/sta->TicketTimeHintInt);
    return PsudoRandBytes(192, seed);
}

// addExtensionRecord, add type, length to extension data
static sds addExtRec(uint8_t typ[2], sds data) {
    sds length = make_sds(2);
    PUT_UINT16_BE(length, (uint16_t)(sdslen(data)));
    sds ret = sdsnewlen(typ, 2);
    appendf(ret, length);
    appendf(ret, data);
    return ret;
}

static void encrypt(uint8_t iv[16], uint8_t key[SHA256_BYTES], 
             uint8_t plaintext[16], uint8_t ciphertext[16]) {
    size_t iv_off = 0;
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (unsigned char *)key, SHA256_BYTES * 8);
    mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_ENCRYPT, 16, &iv_off, iv,
                             (unsigned char *)plaintext, ciphertext);
    mbedtls_aes_free(&aes);
}

static sds MakeRandomField(State* sta) {
    uint8_t goal[SHA256_BYTES];
    uint8_t output[16];
    int t = ((int)time(NULL)) / (12 * 60 * 60);
    sds tohash = sdscatfmt(sdsempty(), "%i%s", t, sta->Key);
    mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 
               (uint8_t *)tohash, sdslen(tohash), (uint8_t *)goal);
    sds iv = CryptoRandBytes(16);
    sds iv_in = sdsdup(iv);
    encrypt((uint8_t *)iv_in, (uint8_t *)sta->AESKey, goal, output);
    sds rest = sdsnewlen(output, 16);
    appendf(iv, rest);
    sdsfree(iv_in);
    sdsfree(tohash);
    return iv;
}

static uint8_t *makeGREASEChrome(uint8_t data[2]) {
    int sixteenth = randIntn(16);
    uint8_t monoGREASE = (uint8_t)(sixteenth*16 + 0xA);
    data[0] = monoGREASE;
    data[1] = monoGREASE;
    return data;
}

static sds makeSupportedGroupsChrome() {
    uint8_t GREASE[2];
    sds suppGroupListLen = make_bytes(0x00, 0x08);
    makeGREASEChrome(GREASE);
    sds suppGroup = sdsnewlen(GREASE, 2);
    sds group = make_bytes(0x00, 0x1d, 0x00, 0x17, 0x00, 0x18);
    appendf(suppGroup, group);
    appendf(suppGroupListLen, suppGroup);
    return suppGroupListLen;
}

static sds composeExtensionsChrome(State* sta) {
    // see https://tools.ietf.org/html/draft-davidben-tls-grease-01
    // This is exclusive to chrome.
    uint8_t GREASE[2];
    sds ext[14];
    ext[0] = addExtRec(makeGREASEChrome(GREASE), sdsempty());                         // First GREASE
    ext[1] = addExtRec(MKBYTE{0xff, 0x01}, make_bytes(0x00));                         // renegotiation_info
    ext[2] = addExtRec(MKBYTE{0x00, 0x00}, makeServerName(sta));                      // server name indication
    ext[3] = addExtRec(MKBYTE{0x00, 0x17}, sdsempty());                               // extended_master_secret
    ext[4] = addExtRec(MKBYTE{0x00, 0x23}, makeSessionTicket(sta));                   // Session tickets
    sds sigAlgo = DecodeHexString("0012040308040401050308050501080606010201");
    ext[5] = addExtRec(MKBYTE{0x00, 0x0d}, sigAlgo);                                  // Signature Algorithms
    ext[6] = addExtRec(MKBYTE{0x00, 0x05}, make_bytes(0x01, 0x00, 0x00, 0x00, 0x00)); // status request
    ext[7] = addExtRec(MKBYTE{0x00, 0x12}, sdsempty());                               // signed cert timestamp
    sds APLN = DecodeHexString("000c02683208687474702f312e31");
    ext[8] = addExtRec(MKBYTE{0x00, 0x10}, APLN);                                     // app layer proto negotiation
    ext[9] = addExtRec(MKBYTE{0x75, 0x50}, sdsempty());                               // channel id
    ext[10] = addExtRec(MKBYTE{0x00, 0x0b}, make_bytes(0x01, 0x00));                  // ec point formats
    ext[11] = addExtRec(MKBYTE{0x00, 0x0a}, makeSupportedGroupsChrome());             // supported groups
    ext[12] = addExtRec(makeGREASEChrome(GREASE), make_bytes(0x00));                  // Last GREASE
    ext[13] = addExtRec(MKBYTE{0x00, 0x15}, makeNullBytes(110-sdslen(ext[2])));       // padding
    sds ret = sdsempty();
    for (int i = 0; i < 14; i++) {
        appendf(ret, ext[i]);
    }
    return ret;
}

static sds composeClientHelloChrome(State* sta) {
    sds clientHello[12];
    clientHello[0] = make_bytes(0x01);                  // handshake type
    clientHello[1] = make_bytes(0x00, 0x01, 0xfc);      // length 508
    clientHello[2] = make_bytes(0x03, 0x03);            // client version
    clientHello[3] = MakeRandomField(sta);              // random
    clientHello[4] = make_bytes(0x20);                  // session id length 32
    clientHello[5] = PsudoRandBytes(32, NowUnixNano()); // session id
    clientHello[6] = make_bytes(0x00, 0x1c);            // cipher suites length 28
    sds cipherSuites = DecodeHexString("2a2ac02bc02fc02cc030cca9cca8c013c014009c009d002f0035000a");
    clientHello[7] = cipherSuites;                      // cipher suites
    clientHello[8] = make_bytes(0x01);                  // compression methods length 1
    clientHello[9] = make_bytes(0x00);                  // compression methods
    clientHello[10] = make_bytes(0x01, 0x97);           // extensions length 407
    clientHello[11] = composeExtensionsChrome(sta);     // extensions
    sds ret = sdsempty();
    for (int i = 0; i < 12; i++) {
        appendf(ret, clientHello[i]);
    }
    return ret;
}

static sds composeExtensionsFireFox(State* sta) {
    sds ext[10];
    ext[0] = addExtRec(MKBYTE{0x00, 0x00}, makeServerName(sta));                      // server name indication
    ext[1] = addExtRec(MKBYTE{0x00, 0x17}, sdsempty());                               // extended_master_secret
    ext[2] = addExtRec(MKBYTE{0xff, 0x01}, make_bytes(0x00));                         // renegotiation_info
    sds suppGroup = DecodeHexString("0008001d001700180019");
    ext[3] = addExtRec(MKBYTE{0x00, 0x0a}, suppGroup);                                // supported groups
    ext[4] = addExtRec(MKBYTE{0x00, 0x0b}, make_bytes(0x01, 0x00));                   // ec point formats
    ext[5] = addExtRec(MKBYTE{0x00, 0x23}, makeSessionTicket(sta));                   // Session tickets
    sds APLN = DecodeHexString("000c02683208687474702f312e31");
    ext[6] = addExtRec(MKBYTE{0x00, 0x10}, APLN);                                     // app layer proto negotiation
    ext[7] = addExtRec(MKBYTE{0x00, 0x05}, make_bytes(0x01, 0x00, 0x00, 0x00, 0x00)); // status request
    sds sigAlgo = DecodeHexString("001604030503060308040805080604010501060102030201");
    ext[8] = addExtRec(MKBYTE{0x00, 0x0d}, sigAlgo);                                  // Signature Algorithms
    ext[9] = addExtRec(MKBYTE{0x00, 0x15}, makeNullBytes(121-sdslen(ext[0])));        // padding
    sds ret = sdsempty();
    for (int i = 0; i < 10; i++) {
        appendf(ret, ext[i]);
    }
    return ret;
}

static sds composeClientHelloFireFox(State* sta) {
    sds clientHello[12];
    clientHello[0] = make_bytes(0x01);                  // handshake type
    clientHello[1] = make_bytes(0x00, 0x01, 0xfc);      // length 508
    clientHello[2] = make_bytes(0x03, 0x03);            // client version
    clientHello[3] = MakeRandomField(sta);              // random
    clientHello[4] = make_bytes(0x20);                  // session id length 32
    clientHello[5] = PsudoRandBytes(32, NowUnixNano()); // session id
    clientHello[6] = make_bytes(0x00, 0x1e);            // cipher suites length 28
    sds cipherSuites = DecodeHexString("c02bc02fcca9cca8c02cc030c00ac009c013c01400330039002f0035000a");
    clientHello[7] = cipherSuites;                      // cipher suites
    clientHello[8] = make_bytes(0x01);                  // compression methods length 1
    clientHello[9] = make_bytes(0x00);                  // compression methods
    clientHello[10] = make_bytes(0x01, 0x95);           // extensions length 405
    clientHello[11] = composeExtensionsFireFox(sta);    // extensions
    sds ret = sdsempty();
    for (int i = 0; i < 12; i++) {
        appendf(ret, clientHello[i]);
    }
    return ret;
}

// ComposeInitHandshake composes ClientHello with record layer
static sds ComposeInitHandshake(State* sta) {
    sds ch;
    if (strcmp(sta->Browser, "chrome") == 0) {
        ch = composeClientHelloChrome(sta);
    } else {
        ch = composeClientHelloFireFox(sta);
    }
    sds ret = AddRecordLayer(ch, MKBYTE{0x16}, MKBYTE{0x03, 0x01});
    return ret;
}

// ComposeReply composes RL+ChangeCipherSpec+RL+Finished
sds ComposeReply() {
    uint8_t TLS12[2] = {0x03, 0x03};
    sds inp = make_bytes(0x01);
    sds ccsBytes = AddRecordLayer(inp, MKBYTE{0x14}, TLS12);
    sds finished = PsudoRandBytes(40, NowUnixNano());
    sds fBytes = AddRecordLayer(finished, MKBYTE{0x16}, TLS12);
    appendf(ccsBytes, fBytes);
    return ccsBytes;
}

// GoQuiet library functions
void go_quiet_setopt(char *opt, int *err)
{
    // init global state
    memset(&sta, 0, sizeof(sta));

    // setup opaque
    sds rands = CryptoRandBytes(32);
    sta.Opaque = BtoInt(rands);
    sdsfree(rands);

    // setup options
    sds optstr = sdsnew(opt);
    int opts_num = 0;
    sds *opts = sdssplitlen(optstr, sdslen(optstr), ";", 1, &opts_num);
    INIT_FIELD(Key);
    INIT_FIELD(TicketTimeHint);
    INIT_FIELD(ServerName);
    INIT_FIELD(Browser);
    for (int i = 0; i < opts_num; i++) {
        sds opt = opts[i];
        SET_FIELD(opt, sta, Key);
        SET_FIELD(opt, sta, TicketTimeHint);
        SET_FIELD(opt, sta, ServerName);
        SET_FIELD(opt, sta, Browser);
    }
    *err = 0;
    CHK_FIELD(*err, Key);
    CHK_FIELD(*err, TicketTimeHint);
    CHK_FIELD(*err, ServerName);
    CHK_FIELD(*err, Browser);
    if (*err == 0) {
        int hint = atoi(sta.TicketTimeHint);
        if (hint == 0) {
            hint = DEFAULT_TIME_HINT;
        }
        sta.TicketTimeHintInt = hint;
    }
    sdsfree(optstr);
    sdsfreesplitres(opts, opts_num);

    // calculate key
    if (*err == 0) {
        SetAESKey(&sta);
    }
}

void go_quiet_freeopt()
{
    sdsfree(sta.Key);
    sdsfree(sta.TicketTimeHint);
    sdsfree(sta.AESKey);
    sdsfree(sta.ServerName);
    sdsfree(sta.Browser);
}

void go_quiet_make_hello(char **data, size_t *out_len)
{
    sds hello = ComposeInitHandshake(&sta);
    *out_len = sdslen(hello);
    *data = malloc(*out_len);
    if (*data != NULL) {
        memcpy(*data, hello, *out_len);
    }
    sdsfree(hello);
}

void go_quiet_make_reply(char **data, size_t *out_len)
{
    sds reply = ComposeReply();
    *out_len = sdslen(reply);
    *data = malloc(*out_len);
    if (*data != NULL) {
        memcpy(*data, reply, *out_len);
    }
    sdsfree(reply);
}
