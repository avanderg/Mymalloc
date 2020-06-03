#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    char *buf = NULL;
    char buf2[24];

    buf = malloc(1000);
    free(buf);
    realloc(buf2, 25);

}
