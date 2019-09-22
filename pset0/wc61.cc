#include <cstdio>
#include <cctype>

int main() {
    size_t b_count = 0, w_count = 0, l_count = 0;
    bool inspace = true;
    while (true) {
        int c = fgetc(stdin);

        if (c == EOF) {
            break;
        }

        b_count++;

        bool thisspace = isspace(c);
        if (inspace && !thisspace) {
            w_count++;
        }

        inspace = thisspace;

        if (c == '\n') {
            l_count++;
        }
        
    }
    
    if (ferror(stdin)) {
        printf("input error\n");
    }

    printf("%8lu %7lu %7lu\n", l_count, w_count, b_count);
    
    return 0;
}