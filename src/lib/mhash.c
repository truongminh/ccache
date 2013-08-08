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

