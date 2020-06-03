#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    char *buf = NULL;
    size_t size = 20;
    size_t nmemb = 15;

    buf = malloc(0);
    if (!buf) {
        printf("malloc(0) is NULL as expected\n");
    }
    else {
        printf("malloc(0) is: %p\n", buf);
    }
    free(buf);

    buf = realloc(NULL,0);
    if (!buf) {
        printf("realloc(NULL,0) is NULL as expected\n");
    }
    else {
        printf("realloc(NULL,0) is: %p\n", buf);
    }
    free(buf);

    buf = calloc(0, size);
    if (!buf) {
        printf("calloc(0, size) is NULL as expected\n");
    }
    else {
        printf("calloc(0, size) is: %p\n", buf);
    }
    free(buf);

    buf = calloc(nmemb, 0);
    if (!buf) {
        printf("calloc(nmemb, 0) is NULL as expected\n");
    }
    else {
        printf("calloc(nmemb, 0) is: %p\n", buf);
    }
    free(buf);
}
