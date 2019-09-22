#include <cstdio>
#include <cstring>

int main(int argc, char const *argv[]) {
    int f_pos = 1;
    for (int i = f_pos + 1; i < argc; i++) {
        int ins_idx = i;
        for (int j = i - 1; j >= f_pos; j--) {
            if (strcmp(argv[ins_idx], argv[j]) < 0) {
                const char* tmp = argv[ins_idx];
                argv[ins_idx] = argv[j];
                argv[j] = tmp;
                ins_idx = j;
            } 
        }
    }

    for (int i = f_pos; i < argc; i++) {
        puts(argv[i]);
    }
    return 0;
}
