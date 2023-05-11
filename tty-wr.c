#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "serial.h"

#define MAX_DATA_LENGTH (256)

static void help(int argc, char **argv)
{
    (void)argc;
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s <device> <baudrate> <byte1> <byte2> [...]\n\n", basename(argv[0]));
    fprintf(stderr, "Exmaple:\n");
    fprintf(stderr, "    %s /dev/tty_mcu 115200 0b 55 aa 00 06 21 06 00 01 ff ff 20\n\n", basename(argv[0]));
}

static char lockfile[128];
static void lockfile_remove(void)
{
    if (lockfile[0])
        unlink(lockfile);
}

static int lockfile_create(void)
{
    int n;

    if (!lockfile[0])
        return 0;

    int fd;
    n = umask(022);
    if ((fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666)) < 0) {
        fprintf(stderr, "fail to create lockfile %s: %s\n", lockfile, strerror(errno));
        return -1;
    } else {
        char buf[12];
        snprintf(buf, sizeof(buf), "%10d\n", getpid());
        buf[sizeof(buf) - 1] = 0;
        if (write(fd, buf, strlen(buf)) < (ssize_t)strlen(buf)) {
            fprintf(stderr, "fail to write lockfile %s: %s\n", lockfile, strerror(errno));
            return -1;
        }
        close(fd);
    }
    umask(n);
    return 0;
}

static void handle_signal(int sig)
{
    (void)sig;
    printf("cleanup...\n");
    lockfile_remove();
}

int main(int argc, char **argv)
{
    serial_t *serial = NULL;
    char *device = NULL;
    uint32_t baudrate = 115200;
    uint8_t tx_data[MAX_DATA_LENGTH];
    uint8_t rx_data[MAX_DATA_LENGTH];
    int i, len, ret;

    if (argc < 4) {
        help(argc, argv);
        exit(1);
    }

    device = argv[1];
    baudrate = strtoul(argv[2], NULL, 10);
    for (i = 3, len = 0; i < argc && len < MAX_DATA_LENGTH; i++, len++) {
        tx_data[len] = strtoul(argv[i], NULL, 16);
    }

    sprintf(lockfile, "/var/lock/%s.lock", basename(device));
    if (lockfile_create() == -1) {
        exit(1);
    }

    /* install signal handler */
    struct sigaction handler;
    handler.sa_handler = handle_signal;
    sigfillset(&handler.sa_mask);
    handler.sa_flags=0;
    sigaction(SIGINT, &handler,0);
    sigaction(SIGTERM, &handler,0);
    sigaction(SIGHUP, &handler,0);

    serial = serial_new();
    if (serial_open(serial, device, baudrate) < 0) {
        fprintf(stderr, "serial_open(): %s\n", serial_errmsg(serial));
        exit(1);
    }

    if (serial_write(serial, tx_data, len) < 0) {
        fprintf(stderr, "serial_write(): %s\n", serial_errmsg(serial));
        exit(1);
    }

    while (1) {
        if ((ret = serial_read(serial, rx_data, 1, 1000)) < 0) {
            fprintf(stderr, "serial_read(): %s\n", serial_errmsg(serial));
            exit(1);
        }
        if (ret > 0) {
            for (int i = 0; i < ret; i++)
            {
                printf("%02x ", rx_data[i]);
            }
        } else {
            //printf("ret = %d\n", ret);
            break;
        }
    }
    printf("\n");

    serial_close(serial);
    serial_free(serial);
    lockfile_remove();
    return 0;
}