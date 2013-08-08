#ifndef __CCACHE_UTIL_H
#define __CCACHE_UTIL_H

int stringmatchlen(const char *p, int plen, const char *s, int slen, int nocase);
int stringmatch(const char *p, const char *s, int nocase);
int stringstartwith(const char *s1, const char *s2);
long long memtoll(const char *p, int *err);
int ll2string(char *s, size_t len, long long value);
int string2ll(char *s, size_t slen, long long *value);
int string2l(char *s, size_t slen, long *value);
int d2string(char *buf, size_t len, double value);
long long ustime(void);
int notsafePath(char *buf);

long long mstime(void);

int utilMkdir(char *dn);
int utilMkSubDirs(char *fullname, int baseoffset);
char *fast_url_encode(const char *str);
int fast_url_decode(const char *str, char *buf);

#endif
