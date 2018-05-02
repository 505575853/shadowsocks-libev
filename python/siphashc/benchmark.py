#!/usr/bin/env python
"""Simple timing benchmark.

Used for testing possible regressions when changing code."""
from __future__ import print_function
import timeit

print('Benchmark (siphash):')
print(timeit.timeit(
    "siphash('0123456789ABCDEF', 'a' * 1000)",
    number=50000,
    setup="from siphashc import siphash"
))
print('Benchmark (md5):')
print(timeit.timeit(
    "hmac.new('0123456789ABCDEF', 'a' * 1000, hashlib.md5).digest()",
    number=50000,
    setup="import hmac, hashlib"
))
print('Benchmark (sha1):')
print(timeit.timeit(
    "hmac.new('0123456789ABCDEF', 'a' * 1000, hashlib.sha1).digest()",
    number=50000,
    setup="import hmac, hashlib"
))