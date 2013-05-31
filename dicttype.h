#ifndef DICTTYPE_H
#define DICTTYPE_H

#include "dict.h"

int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2);
void dictSdsDestructor(void *privdata, void *val);
unsigned int dictSdsHash(const void *key);

/* sds dict, keys and values are sds strings. */
dictType sdsDictType;
dictType keylistDictType;
dictType objSdsDictType;

#endif // DICTTYPE_H
