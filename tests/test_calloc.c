#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    int i;
    char *buf = NULL;

    buf = calloc(14, sizeof(char));
    strcpy(buf, "Hello World!\n");
    printf("%s", buf);

    free(buf);

    buf = calloc(128, sizeof(char));

    for (i=0; i<(127-32); i++) {
        buf[i] = (char) i+32;
    }

    printf("%s\n", buf);
}

