#include <cstdio>

int main() {
    unsigned long count = 0;
    while (fgetc(stdin) != EOF) {
        count++;
    }

    if (ferror(stdin)) {
        printf("input error\n");
    }

    printf("%8lu\n", count);
    
    return 0;
}