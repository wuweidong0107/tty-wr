#include <stdio.h>
#include <stdint.h>
#include <libgen.h>
#include <stdlib.h>

#define MAX_DATA_LENGTH (1024)

static uint8_t gen_checkcode(const void *cp_data, size_t length)
{
    const uint8_t *cp_input = (uint8_t*)cp_data;
    uint8_t result = cp_input[0];
    for (size_t i = 1; i < length; ++i)
        result ^= cp_input[i];
    return result;
}

static void help(int argc, char **argv)
{
    (void)argc;
    char *app = basename(argv[0]);
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s <byte1> <byte2> [...]\n\n", app);
    fprintf(stderr, "Exmaple:\n");
    fprintf(stderr, "    %s 11 22 33 44 55\n", app);

}
int main(int argc, char** argv)
{

    uint8_t data[MAX_DATA_LENGTH];
    int i, len, ret;

    if (argc < 2) {
        help(argc, argv);
        exit(1);
    }

    for (i = 1, len = 0; i < argc && len < MAX_DATA_LENGTH; i++, len++) {
        data[len] = strtoul(argv[i], NULL, 16);
    }
    printf("%x\n", gen_checkcode(data, len));
    return 0;
}