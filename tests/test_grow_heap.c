#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    char *buf;
    char *buf2;

    buf = malloc(65480);
    buf2 = malloc(15);
    free(buf);
    free(buf2);
}
