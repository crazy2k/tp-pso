#include <utils_common.h>


#define HEX_BASE 16
#define HEX_LOG2BASE 4
#define HEX_REPLENGTH (sizeof(uint32_t)*8/HEX_LOG2BASE)


void *memcpy(void *dest, const void *src, size_t n) {
    char *bdest = (char *) dest;
    char *bsrc = (char *) src;
    int i;

    for (i = 0; i < n; i++)
        bdest[i] = bsrc[i];

    return dest;
}

void *memset(void *s, int c, size_t n) {
    char *bs = (char *)s;
    int i;

    for (i = 0; i < n; i++)
        bs[i] = (char)c;

    return s;
}

int strcmp(const char * src, const char * dst) {
    int ret = 0 ;

    while(!(ret = (unsigned int)*src - (unsigned int)*dst) && *dst) {
        ++src;
        ++dst;
    }

    if (ret < 0)
        ret = -1;
    else if (ret > 0)
        ret = 1;

    return ret;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    unsigned char uc1, uc2;

    if (n == 0)
        return 0;

    while (n-- > 0 && *s1 == *s2) {
        if (n == 0 || *s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    uc1 = (*(unsigned char *) s1);
    uc2 = (*(unsigned char *) s2);
    return ((uc1 < uc2) ? -1 : (uc1 > uc2));
}

char *strchr(const char *s, int c) {
    while (*s != (char)c)
        if (!*s++)
            return 0;
    return (char *)s;
}

char *strstr(const char *in, const char *str) {
    char c;
    size_t len;

    c = *str++;
    if (!c)
        return (char *) in;

    len = strlen(str);
    do {
        char sc;

        do {
            sc = *in++;
            if (!sc)
                return (char *) 0;
        } while (sc != c);
    } while (strncmp(in, str, len) != 0);

    return (char *) (in - 1);
}

int strlen(const char* str) {
    int result = 0;

    if (str)
        while(str[result]) result++;

    return result;
}

char *strcpy(char *s1, const char *s2) {
    char *dst = s1;
    const char *src = s2;

    while ((*dst++ = *src++) != '\0')
        ;

    return s1;
}

char *strcat(char *s1, const char *s2) {
    char *s = s1;

    while (*s != '\0')
        s++;

    strcpy(s, s2);
    return s1;
}

char *strncat(char *s1, const char *s2, size_t n) {
    char *s = s1;

    while (*s != '\0')
        s++;

    while (n != 0 && (*s = *s2++) != '\0') {
        n--;
        s++;
    }
    if (*s != '\0')
        *s = '\0';
    return s1;
}

int strtoi(const char *str) {
    int num = 0, i;
    bool neg = FALSE;

    while (*str == ' ') str++;

    for (i = 0; i <= strlen(str); i++) {
        if (str[i] >= '0' && str[i] <= '9')
            num = num * 10 + str[i] - '0';
        else if (i == 0 && str[0] == '-')
            neg = TRUE;
        else
            break;
    }

    return neg ? -num : num;
}

void itohex(uint32_t n, char *str) {
    char chars[HEX_BASE],
         base = '0';

    int i;
    for (i = 0; i < HEX_BASE; i++) {
        if (i == 10)
            base = 'A';

        chars[i] = base + (i % 10);
    }

    for (i = HEX_REPLENGTH - 1; i >= 0; i--, n /= HEX_BASE)
        str[i] = chars[n % HEX_BASE];

    str[HEX_REPLENGTH] = '\0';
}

void itostr(int n, char *s) {
    int i, sign;

    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void reverse(char *s) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

int power(int x, int y) {
    int res = 1;
    int i;

    for (i = 0; i < y; i++)
        res *= x;
    return res;
}

long max(long var1, long var2) {
    return var1 < var2 ? var2 : var1;
}

int isdigit(int var) {
    return (char)var >= '0' && (char)var <= '9';
}

int isnumeric(const char *str) {
  while(*str)
  {
    if(!isdigit(*str))
      return FALSE;
    str++;
  }

  return TRUE;
}

long min(long var1, long var2) {
    return var1 > var2 ? var2 : var1;
}

int align_to_lower(int number, int mult) {
   return ((number / mult)) * mult;
}

int align_to_next(int number, int mult) {
   return ((number / mult) + (number % mult > 0 ? 1 : 0)) * mult;
}

