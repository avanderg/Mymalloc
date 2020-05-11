#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char *argv[]) {
    char *buf = NULL;
    char *buf2 = NULL;
    char *buf3 = NULL;
    buf = realloc(buf, 20000);
    buf = realloc(buf, 5);
    buf = realloc(buf, 300000);
    buf = realloc(buf, 0);
    buf = realloc(buf, 100000);
    buf2 = realloc(buf2, 5000);
    buf3 = calloc(500, sizeof(char));
    strcpy(buf3, "hello world\n");
    strcpy(buf3+495, "end\n");
    printf("Buf3 stuff: should be 'hello world' then 'end'\n");
    printf("%s", buf3);
    printf("%s", buf3+495);
    buf[0] = 'a';
    buf[99999] = 'b';
    buf2[0] = 'c';
    buf2[4999] = 'd';

    printf("Buf stuff: should be 'a' then 'b\n");
    printf("%c\n", buf[0]);
    printf("%c\n", buf[99999]);

    printf("Buf2 stuff: should be 'c' then 'd\n");
    printf("%c\n", buf2[0]);
    printf("%c\n", buf2[4999]);
    printf("Buf3 stuff: should be 'hello world' then 'end'\n");
    printf("%s", buf3);
    printf("%s", buf3+495);
    printf("Buf stuff: should be 'a' then 'b\n");
    printf("%c\n", buf[0]);
    printf("%c\n", buf[99999]);
    free(buf2);
    buf2 = realloc(buf, 2104567890);
    if (!buf2) {
        printf("Failed realloc\n");
        if (errno == ENOMEM) {
            printf("No memory left\n");
        }
    }
    free(buf);
    buf = malloc(400000);
    buf = realloc(buf, 35);
    buf = realloc(buf, 512);
    free(buf3);
    buf3 = malloc(300000);
    buf = realloc(buf, 100000);
    free(buf);
    free(buf3);
}
