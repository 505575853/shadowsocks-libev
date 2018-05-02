#!/usr/bin/env python

"""
    Copyright (C) 2012 Bo Zhu http://about.bozhu.me
    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
"""

import struct
import binascii
try:
    import siphashc
except ImportError:
    siphashc = None

_siphashc = siphashc

class SipHash:
    def __init__(self, key, msg):
        self.msg = msg
        self.secret = key
        if not siphashc:
            self.auth_func = self.auth
            key_len = len(key)
            if key_len != 16:
                in_key = key
                if key_len < 16:
                    in_key += b'\x00' * (16 - key_len)
                key_hash = struct.pack('<Q', self.auth_func(in_key[:16], key))
                self.secret = key_hash + key_hash
        else:
            self.auth_func = siphashc.siphash

    def digest(self):
        return struct.pack('<Q', self.auth_func(self.secret, self.msg))

    def hexdigest(self):
        return binascii.hexlify(self.digest())

    def __process_message(self, message):
        self.__message = []

        length = len(message)
        for start in xrange(0, length - 7, 8):
            state = ord(message[start]) \
                | (ord(message[start + 1]) << 8)  \
                | (ord(message[start + 2]) << 16) \
                | (ord(message[start + 3]) << 24) \
                | (ord(message[start + 4]) << 32) \
                | (ord(message[start + 5]) << 40) \
                | (ord(message[start + 6]) << 48) \
                | (ord(message[start + 7]) << 56)
            self.__message.append(state)

        start = (length // 8) * 8
        state = (length % 256) << 56
        for i in range(length - start):
            state |= (ord(message[start + i]) << (i * 8))
        self.__message.append(state)

    def __SipRound(self):
        self.__v0 += self.__v1  # no need to mod 2^64 now
        self.__v2 += self.__v3
        self.__v1 = (self.__v1 << 13) | (self.__v1 >> 51)
        self.__v3 = (self.__v3 << 16) | (self.__v3 >> 48)
        self.__v1 ^= self.__v0
        self.__v3 ^= self.__v2
        self.__v0 = (self.__v0 << 32) | ((self.__v0 >> 32) & 0xffffffff)
        self.__v2 += self.__v1
        self.__v0 += self.__v3
        self.__v0 &= 0xffffffffffffffff
        self.__v1 = (self.__v1 << 17) | ((self.__v1 >> 47) & 0x1ffff)
        self.__v3 = ((self.__v3 & 0x7ffffffffff) << 21) \
            | ((self.__v3 >> 43) & 0x1fffff)
        self.__v1 ^= self.__v2
        self.__v1 &= 0xffffffffffffffff
        self.__v3 ^= self.__v0
        self.__v2 = ((self.__v2 & 0xffffffff) << 32) \
            | ((self.__v2 >> 32) & 0xffffffff)

    def auth(self, key, message):
        k0, k1 = struct.unpack('<QQ', key)

        # initialization
        self.__v0 = k0 ^ 0x736f6d6570736575
        self.__v1 = k1 ^ 0x646f72616e646f6d
        self.__v2 = k0 ^ 0x6c7967656e657261
        self.__v3 = k1 ^ 0x7465646279746573

        self.__process_message(message)

        # compression
        for m in self.__message:
            self.__v3 ^= m
            self.__SipRound()
            self.__v0 ^= m

        # finalization
        self.__v2 ^= 0xff
        self.__SipRound()
        self.__SipRound()
        self.__SipRound()
        return self.__v0 ^ self.__v1 ^ self.__v2 ^ self.__v3

def new(key, msg):
    return SipHash(key, msg)

if __name__ == '__main__':
    if not _siphashc:
        print("Warning: fallback to slow siphash implementation")

    def HASH_TEST(key, msg, result):
        ret = new(key, msg).hexdigest()
        print(ret + (': OK' if ret == result else (': FAILED <--' + result)))

    HASH_TEST("0123456789ABCDEF", "a", "6962dc810c7ddbc8")
    HASH_TEST("0123456789ABCDEF", "b", "2ca0e922eb19841d")
    HASH_TEST("0102030405060708", "c", "1f0be1380647ff32")
    HASH_TEST("0102030405060708", "d", "6399ed76dc57bd18")
    HASH_TEST("0123456789AB", "a", "eb7c225a7e606982")
    HASH_TEST("0123456789AB\x00", "a", "76afa8a2243f1b7d")
    HASH_TEST("0123456789ABCDEFGHIJ", "a", "0f691f65634add99")
    HASH_TEST("0123456789ABCDEFGHIK", "a", "9f979a8531f712d4")
