#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello bella pe' te\n");

    for (int i=0; i<argc; printf("- %s\n", argv[i++]));

    return 0;
}
