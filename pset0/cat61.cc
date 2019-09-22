#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>

void _cat(const char *fname) {
    FILE *fp = NULL;
    char buf[BUFSIZ];
    if (strcmp(fname, "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(fname, "r");
    }

    if (!fp) {
        fprintf(stderr, "cat: %s: %s\n", fname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (!feof(fp) && !ferror(fp) && !ferror(stdout)) {
        size_t n_read = fread(buf, sizeof(char), BUFSIZ, fp);
        (void) fwrite(buf, 1, n_read, stdout);
    }

    if (ferror(fp) || ferror(stdout)) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fp != stdin) {
        fclose(fp);
    }
}

int main(int argc, char const *argv[])
{
    if (argc == 1) {
        _cat("-");
    } else {
        for (int i = 1; i < argc; i++) {
            _cat(argv[i]);
        }
    }
    return 0;
}
