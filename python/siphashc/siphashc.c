/*
 * Copyright (c) 2013 Eli Janssen
 * Copyright (c) 2014 Carlo Pires
 * Copyright (c) 2017 Michal Čihař
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
**/

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

#include <stdlib.h>
#include <string.h>
#include <Python.h>

#include "siphash.c"

static PyObject *pysiphash(PyObject *self, PyObject *args) {
    const char *key = NULL;
    int key_sz;
    const char *plaintext = NULL;
    int plain_sz;
    union {
        uint8_t bytes[8];
        uint64_t num;
    } hash; // 64-bit output

    if (!PyArg_ParseTuple(
            args, "s#s#:siphash",
            &key, &key_sz, &plaintext, &plain_sz)) {
        return NULL;
    }

    ss_fast_hash_with_key(
        (char *)hash.bytes,
        (char*)plaintext,
        plain_sz,
        (uint8_t *)key,
        key_sz);

    return PyLong_FromUnsignedLongLong(hash.num);
}

static char siphash_docstring[] = ""
    "Computes Siphash-2-4 of the given string and key\n\n"
    "siphash(key, plaintext) -> hash\n"
    " - key: must be 128 bit long (16 chars at 8 bit each)\n"
    " - plaintext: text\n"
    "returns 64-bit output (python Long)\n";

static PyMethodDef siphashc_methods[] = {
    {"siphash", pysiphash, METH_VARARGS, siphash_docstring},
    {NULL, NULL, 0, NULL} /* sentinel */
};

#if PY_MAJOR_VERSION >= 3
	static struct PyModuleDef moduledef = {
		    PyModuleDef_HEAD_INIT,
		    "siphashc",
		    NULL,
            -1,
		    siphashc_methods,
		    NULL,
		    NULL,
		    NULL,
		    NULL
	};

	#define INITERROR return NULL

	PyObject *
	PyInit_siphashc(void)
#else
	#define INITERROR return

	void
	initsiphashc(void)
#endif
{
    PyObject *module;
#if PY_MAJOR_VERSION >= 3
    module = PyModule_Create(&moduledef);
#else
    module = Py_InitModule("siphashc", siphashc_methods);
#endif

    if (module == NULL)
        INITERROR;

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

