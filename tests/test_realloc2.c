#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char *argv[]) {
    char *buf = NULL;
    char *buf2 = NULL;
    char *buf3 = NULL;
    char *buf4 = NULL;

    buf = malloc(16);
    buf[15] = '3';
    buf2 = malloc(29);
    buf3 = malloc(100);
    buf4 = malloc(20);
    free(buf3);

    buf3 = realloc(buf, 25);
    printf("%c\n", buf3[15]);
}

