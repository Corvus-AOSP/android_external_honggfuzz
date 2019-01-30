#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "libhfcommon/common.h"
#include "libhfuzz/instrument.h"

__attribute__((visibility("default"))) __attribute__((used))
const char* const LIBHFUZZ_module_memorycmp = "LIBHFUZZ_module_memorycmp";

#define RET_CALL_CHAIN (uintptr_t) __builtin_return_address(0)

static inline int HF_strcmp(const char* s1, const char* s2, uintptr_t addr) {
    unsigned int v = 0;
    for (size_t i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0' || s2[i] == '\0') {
            break;
        }
        v++;
    }
    instrumentUpdateCmpMap(addr, v);
    return __builtin_strcmp(s1, s2);
}

static inline int HF_strcasecmp(const char* s1, const char* s2, uintptr_t addr) {
    unsigned int v = 0;
    for (size_t i = 0; tolower((unsigned char)s1[i]) == tolower((unsigned char)s2[i]); i++) {
        if (s1[i] == '\0' || s2[i] == '\0') {
            break;
        }
        v++;
    }
    instrumentUpdateCmpMap(addr, v);
    return __builtin_strcasecmp(s1, s2);
}

static inline int HF_strncmp(const char* s1, const char* s2, size_t n, uintptr_t addr) {
    unsigned int v = 0;
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            break;
        }
        if (s1[i] == '\0' || s2[i] == '\0') {
            break;
        }
        v++;
    }
    instrumentUpdateCmpMap(addr, v);
    return __builtin_strncmp(s1, s2, n);
}

static inline int HF_strncasecmp(const char* s1, const char* s2, size_t n, uintptr_t addr) {
    unsigned int v = 0;
    for (size_t i = 0; i < n; i++) {
        if (tolower((unsigned char)s1[i]) != tolower((unsigned char)s2[i])) {
            break;
        }
        if (s1[i] == '\0' || s2[i] == '\0') {
            break;
        }
        v++;
    }
    instrumentUpdateCmpMap(addr, v);
    return __builtin_strncasecmp(s1, s2, n);
}

static inline char* HF_strstr(const char* haystack, const char* needle, uintptr_t addr) {
    size_t needle_len = strlen(needle);
    for (size_t i = 0; haystack[i]; i++) {
        if (HF_strncmp(&haystack[i], needle, needle_len, addr) == 0) {
            break;
        }
    }
    return __builtin_strstr(haystack, needle);
}

static inline char* HF_strcasestr(const char* haystack, const char* needle, uintptr_t addr) {
    size_t needle_len = strlen(needle);
    for (size_t i = 0; haystack[i]; i++) {
        if (HF_strncasecmp(&haystack[i], needle, needle_len, addr) == 0) {
            return (char*)(&haystack[i]);
        }
    }
    return NULL;
}

static inline int HF_memcmp(const void* m1, const void* m2, size_t n, uintptr_t addr) {
    const unsigned char* s1 = (const unsigned char*)m1;
    const unsigned char* s2 = (const unsigned char*)m2;

    unsigned int v = 0;
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            break;
        }
        v++;
    }

    instrumentUpdateCmpMap(addr, v);
    return __builtin_memcmp(m1, m2, n);
}

static inline void* HF_memmem(const void* haystack, size_t haystacklen, const void* needle,
    size_t needlelen, uintptr_t addr) {
    if (needlelen > haystacklen) {
        return NULL;
    }
    if (needlelen == 0) {
        return (void*)haystack;
    }

    const char* h = haystack;
    for (size_t i = 0; i <= (haystacklen - needlelen); i++) {
        if (HF_memcmp(&h[i], needle, needlelen, addr) == 0) {
            return (void*)(&h[i]);
        }
    }
    return NULL;
}

static inline char* HF_strcpy(char* dest, const char* src, uintptr_t addr) {
    uint32_t len = strlen(src);
    if (len > 0) {
        /* The longer the 'src's len, the better */
        instrumentUpdateCmpMap(addr, (sizeof(len) * 8) - __builtin_clz(len));
    }
    return __builtin_strcpy(dest, src);
}

/* Define a weak function x, as well as __wrap_x pointing to x */
#define XVAL(x) x
#define HF_WEAK_WRAP(ret, func, ...) \
    _Pragma(HF__XSTR(weak func = __wrap_##func)) XVAL(ret) XVAL(__wrap_##func)(__VA_ARGS__)

/* Typical libc wrappers */
HF_WEAK_WRAP(int, strcmp, const char* s1, const char* s2) {
    return HF_strcmp(s1, s2, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(int, strcasecmp, const char* s1, const char* s2) {
    return HF_strcasecmp(s1, s2, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(int, strncmp, const char* s1, const char* s2, size_t n) {
    return HF_strncmp(s1, s2, n, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(int, strncasecmp, const char* s1, const char* s2, size_t n) {
    return HF_strncasecmp(s1, s2, n, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(char*, strstr, const char* haystack, const char* needle) {
    return HF_strstr(haystack, needle, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(char*, strcasestr, const char* haystack, const char* needle) {
    return HF_strcasestr(haystack, needle, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(int, memcmp, const void* m1, const void* m2, size_t n) {
    return HF_memcmp(m1, m2, n, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(int, bcmp, const void* m1, const void* m2, size_t n) {
    return HF_memcmp(m1, m2, n, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(
    void*, memmem, const void* haystack, size_t haystacklen, const void* needle, size_t needlelen) {
    return HF_memmem(haystack, haystacklen, needle, needlelen, RET_CALL_CHAIN);
}
HF_WEAK_WRAP(void*, strcpy, char* dest, const char* src) {
    return HF_strcpy(dest, src, RET_CALL_CHAIN);
}

/*
 * Apache's httpd wrappers
 */
HF_WEAK_WRAP(int, ap_cstr_casecmp, const char* s1, const char* s2) {
    return HF_strcasecmp(s1, s2, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, ap_cstr_casecmpn, const char* s1, const char* s2, size_t n) {
    return HF_strncasecmp(s1, s2, n, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(const char*, ap_strcasestr, const char* s1, const char* s2) {
    return HF_strcasestr(s1, s2, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, apr_cstr_casecmp, const char* s1, const char* s2) {
    return HF_strcasecmp(s1, s2, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, apr_cstr_casecmpn, const char* s1, const char* s2, size_t n) {
    return HF_strncasecmp(s1, s2, n, RET_CALL_CHAIN);
}

/*
 * *SSL wrappers
 */
HF_WEAK_WRAP(int, CRYPTO_memcmp, const void* m1, const void* m2, size_t len) {
    return HF_memcmp(m1, m2, len, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, OPENSSL_memcmp, const void* m1, const void* m2, size_t len) {
    return HF_memcmp(m1, m2, len, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, OPENSSL_strcasecmp, const char* s1, const char* s2) {
    return HF_strcasecmp(s1, s2, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, OPENSSL_strncasecmp, const char* s1, const char* s2, size_t len) {
    return HF_strncasecmp(s1, s2, len, RET_CALL_CHAIN);
}

/*
 * libXML wrappers
 */
HF_WEAK_WRAP(int, xmlStrncmp, const char* s1, const char* s2, int len) {
    if (len <= 0) {
        return 0;
    }
    if (s1 == s2) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return HF_strncmp(s1, s2, (size_t)len, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, xmlStrcmp, const char* s1, const char* s2) {
    if (s1 == s2) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return HF_strcmp(s1, s2, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, xmlStrEqual, const char* s1, const char* s2) {
    if (s1 == s2) {
        return 1;
    }
    if (s1 == NULL) {
        return 0;
    }
    if (s2 == NULL) {
        return 0;
    }
    if (HF_strcmp(s1, s2, RET_CALL_CHAIN) == 0) {
        return 1;
    }
    return 0;
}

HF_WEAK_WRAP(int, xmlStrcasecmp, const char* s1, const char* s2) {
    if (s1 == s2) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return HF_strcasecmp(s1, s2, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(int, xmlStrncasecmp, const char* s1, const char* s2, int len) {
    if (len <= 0) {
        return 0;
    }
    if (s1 == s2) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return HF_strncasecmp(s1, s2, (size_t)len, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(const char*, xmlStrstr, const char* haystack, const char* needle) {
    if (haystack == NULL) {
        return NULL;
    }
    if (needle == NULL) {
        return NULL;
    }
    return HF_strstr(haystack, needle, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(const char*, xmlStrcasestr, const char* haystack, const char* needle) {
    if (haystack == NULL) {
        return NULL;
    }
    if (needle == NULL) {
        return NULL;
    }
    return HF_strcasestr(haystack, needle, RET_CALL_CHAIN);
}

/*
 * Samba wrappers
 */
HF_WEAK_WRAP(int, memcmp_const_time, const void* s1, const void* s2, size_t n) {
    return HF_memcmp(s1, s2, n, RET_CALL_CHAIN);
}

HF_WEAK_WRAP(bool, strcsequal, const void* s1, const void* s2) {
    if (s1 == s2) {
        return true;
    }
    if (!s1 || !s2) {
        return false;
    }
    return (HF_strcmp(s1, s2, RET_CALL_CHAIN) == 0);
}
