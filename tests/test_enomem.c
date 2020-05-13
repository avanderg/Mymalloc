#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char *argv[]) {
    char *buf = NULL;
    char *buf2 = NULL;
    char *buf3 = NULL;
    /*

    buf = realloc(buf, 100);
    buf2 = realloc(buf, 2104567890);
    if (!buf2) {
        printf("Failed realloc\n");
        if (errno == ENOMEM) {
            printf("No memory left\n");
            errno=0;
        }
    }
    free(buf);
    */
    buf = malloc(400000);
    buf3 = realloc(buf3, 1);
    free(buf);
    free(buf3);


    #if defined (__x86_64)
    buf = malloc(210456789000000);
    #else
    buf = malloc(2104567890);
    #endif
    printf("finished malloc\n");
    if (!buf) {
        printf("Failed malloc\n");
        if (errno == ENOMEM) {
            printf("No memory left\n");
            errno=0;
        }
    }
    printf("About to malloc buf2\n");
    buf2 = malloc(300000);
    free(buf);
    free(buf2);

    buf = calloc(2104567890, 1);
    if (!buf) {
        printf("Failed calloc\n");
        if (errno == ENOMEM) {
            printf("No memory left\n");
            errno=0;
        }
    }
    buf2 = malloc(300000);
    /*free(buf); */
    free(buf2);

    buf = realloc(buf, 2104567890);
    if (!buf) {
        printf("Failed realloc\n");
        if (errno == ENOMEM) {
            printf("No memory left\n");
            errno=0;
        }
    }
    buf2 = malloc(300000);
    free(buf);
    free(buf2);

}

