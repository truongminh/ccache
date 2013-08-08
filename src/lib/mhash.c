/* mhash.c - An implementation of Bernstein hash algorithm
 *
 * Copyright (c) 2013, Nguyen Truong Minh <nguyentrminh at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "mhash.h"
#include <stdint.h>
#include <malloc.h>
#include <string.h>

#define _TMHASH uint32_t
// #define _TMHASH uint64_t
#define _TMHASH_BYTE_SIZE sizeof(_TMHASH)
#define _TMHASH_ALIGN 4

static const char bin2hex[] = { '0', '1', '2', '3',
                                '4', '5', '6', '7',
                                '8', '9', 'a', 'b',
                                'c', 'd', 'e', 'f' };

/* Generic hash function (a popular one from Bernstein).
 * I tested a few and this was the best. */
char *mhashFunction(const unsigned char *buf, int len) {
    _TMHASH hash = 5381;
    while (len--)
        hash = ((hash << 5) ^ hash) ^ (*buf++); /* hash * 33 + c */
    unsigned char *vhash = (unsigned char *)&hash;
    char *result = (char*)memalign(_TMHASH_ALIGN,_TMHASH_BYTE_SIZE+3);
    char *ptr = result;
    ptr[_TMHASH_BYTE_SIZE] = '\0';
#if (_TMHASH == uint32_t)
    /* first byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = '/';
    *ptr++ = bin2hex[*vhash++ & 0xf];    
    /* second byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = '/';
    *ptr++ = bin2hex[*vhash++ & 0xf];

    /* third byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    /* fourth byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
#elif(_TMHASH == uint64_t)
    /* first byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    *ptr++ = '/';
    /* second byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    *ptr++ = '/';
    /* third byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    /* fourth byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    /* fifith byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    /* sixth byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    /* seventh byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
    /* eight byte */
    *ptr++ = bin2hex[*vhash >> 4];
    *ptr++ = bin2hex[*vhash++ & 0xf];
#endif
    return result;
}

