"""Test for siphashc module."""
import unittest
import binascii
import struct
from siphashc import siphash


def hexify(result):
    return binascii.hexlify(struct.pack('<Q', result))

class TestSiphashC(unittest.TestCase):
    """Test for siphashc module."""
    def test_hash(self):
        def HASH_TEST(k, m, r):
            self.assertEqual(r, hexify(siphash(k, m)))

        HASH_TEST("0123456789ABCDEF", "a", "6962dc810c7ddbc8")
        HASH_TEST("0123456789ABCDEF", "b", "2ca0e922eb19841d")
        HASH_TEST("0102030405060708", "c", "1f0be1380647ff32")
        HASH_TEST("0102030405060708", "d", "6399ed76dc57bd18")
        HASH_TEST("0123456789AB", "a", "eb7c225a7e606982")
        HASH_TEST("0123456789AB\x00", "a", "76afa8a2243f1b7d")
        HASH_TEST("0123456789ABCDEFGHIJ", "a", "0f691f65634add99")
        HASH_TEST("0123456789ABCDEFGHIK", "a", "9f979a8531f712d4")
