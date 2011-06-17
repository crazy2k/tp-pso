#include <user/utils.h>

int strlen(char* str) {
    int result = 0;

    if (str)
        while(str[result]) result++;

    return result;
}

void *memcpy(void *dest, const void *src, int n) {
    char *bdest = (char *) dest;
    char *bsrc = (char *) src;
    int i;

    for (i = 0; i < n; i++) {
        bdest[i] = bsrc[i];
    }

    return dest;
}


int strcmp(char * src, char * dst) {
    int ret = 0 ;

    while(!(ret = (unsigned int)*src - (unsigned int)*dst) && *dst) {
        src++;
        dst++;
    }

    if (ret < 0)
        ret = -1 ;
    else if (ret > 0)
        ret = 1 ;

    return ret;
}

int power(int x, int y) {
    int res = 1;
    int i;

    for (i = 0; i < y; i++)
        res *= x;
    return res;
}

int strtoi(char *str) {
    int zeroes = strlen(str) - 1;
    int number = 0;
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        int d = str[i] - '0';
        number += d*power(10, zeroes);
        zeroes--;
    }
    return number;

}
