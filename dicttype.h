#ifndef DICTTYPE_H
#define DICTTYPE_H

#include "sds.h"
#include "dict.h"
#include <malloc.h> /* define NULL value */

int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2);
void dictSdsDestructor(void *privdata, void *val);
unsigned int dictSdsHash(const void *key);

/* sds dict, keys and values are sds strings. */
dictType sdsDictType;
dictType keylistDictType;

#endif // DICTTYPE_H
