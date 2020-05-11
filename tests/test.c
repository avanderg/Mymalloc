#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    char *buf = NULL;
    char *buf2;
    char *buf3;
    char *buf4;
    char *buf5;
    char *buf6;
    char *buf7;

    buf = malloc(1000);
    buf4 = malloc(2);
    free(buf);
    buf2 = malloc(20);
    buf5 = malloc(10);
    buf3 = malloc(400);
    buf6 = malloc(830);


    buf7 = malloc(500000);
    free(buf4);
    free(buf2);
    free(buf5);
    free(buf3);
    free(buf6);
    free(buf7);
    buf = calloc(14, sizeof(char));
    strcpy(buf, "Hello World!\n");
    printf("%s", buf);
    buf2 = malloc(32);
    buf3 = malloc(48);

}
