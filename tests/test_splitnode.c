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

    buf = malloc(1000);
    buf2 = malloc(2);
    free(buf);
    buf3 = malloc(20);
    buf4 = malloc(10);
    buf5 = malloc(400);
    buf6 = malloc(830);
    free(buf2);
    free(buf3);
    free(buf4);
    free(buf5);
    free(buf6);
}
