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
    char *app = basename(argv[0]);
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s <device> <baudrate> <byte1> <byte2> [...]\n\n", app);

    fprintf(stderr, "Env:\n");
    fprintf(stderr, "    PROT=aimy, protocal\n");  
    fprintf(stderr, "    LOOP=100, loop count\n");  
    fprintf(stderr, "    LOOP_DELAY=100, loop delay, in ms\n\n");

    fprintf(stderr, "Exmaple:\n");
    fprintf(stderr, "    %s /dev/tty_mcu 115200                                         # monitor mode\n", app);
    fprintf(stderr, "    %s /dev/tty_mcu 115200 0b 55 aa 00 06 21 06 00 01 ff ff 20\n", app);
    fprintf(stderr, "    %s /dev/tty_58g 57600 55 aa 01 50 00 00                        # 58g read config\n", app);
    fprintf(stderr, "    %s /dev/tty_58g 57600 55 aa 01 51 00 00                        # 58g read rf status\n", app);
    fprintf(stderr, "    %s /dev/tty_bp1048 460800 06 55 AA 80 01 A1 20\n", app);  
    fprintf(stderr, "    PROT=aimy %s /dev/tty_bp1048 460800 06 55 AA 80 01 A1 20\n", app);
    fprintf(stderr, "    LOOP=100 LOOP_DELAY=1000 PROT=aimy %s /dev/tty_bp1048 460800 06 55 AA 80 01 A1 20   # test 100 tims\n", app);
}

static char lockfile[128];
static void lockfile_remove(void)
{
    if (lockfile[0]) {
        unlink(lockfile);
        sync();
    }
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
            //fprintf(stderr, "fail to write lockfile %s: %s\n", lockfile, strerror(errno));
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
    exit(1);
}

int main(int argc, char **argv)
{
    serial_t *serial = NULL;
    char *device = NULL;
    uint32_t baudrate = 115200;
    uint8_t tx_data[MAX_DATA_LENGTH];
    uint8_t rx_data[MAX_DATA_LENGTH];
    int i, len, ret;
    int wait_timeout_ms = 10000;
    bool first_byte = true;
    bool fist_byte_is_len = false;
    char *prot=getenv("PROT");
    char *loop_env=getenv("LOOP");
    int loop = 1;
    
    char *loop_delay_env=getenv("LOOP_DELAY");
    int loop_delay = 0;                                         // ms
    
    if (loop_env)
        loop = strtoul(loop_env, NULL, 10);
    if (loop < 1)
        loop = 1;

    if (loop_delay_env)
        loop_delay = strtoul(loop_delay_env, NULL, 10) * 1000;
    if (loop_delay < 0)
        loop_delay = 0;

    if (argc < 3) {
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
        lockfile[0] = 0;
        //fprintf(stderr, "WARN: no lockfile\n");
    }

    setbuf(stdout, NULL);

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
        goto err2;
    }

    while(loop--) {
        usleep(loop_delay);
        if (len > 0) {
            if (serial_write(serial, tx_data, len) < 0) {
                fprintf(stderr, "serial_write(): %s\n", serial_errmsg(serial));
                goto err1;
            }        
        }

        if (prot != NULL && !strcmp(prot, "aimy")) {
            fist_byte_is_len = true;
        }

        int rx_total=-1, rx_len=0;
        first_byte = true;
        while (1) {
            if (len == 0 ) {
                wait_timeout_ms = -1;
            } else {
                wait_timeout_ms = 3000;
            }

            if ((ret = serial_read(serial, rx_data, 1, wait_timeout_ms)) < 0) {
                fprintf(stderr, "serial_read(): %s\n", serial_errmsg(serial));
                goto err1;
            }
            if (ret > 0) {
                if (first_byte == true && fist_byte_is_len == true) {
                    rx_total = rx_data[0]+1;
                    first_byte = false;
                }
                for (int i = 0; i < ret; i++) {
                    printf("%02x ", rx_data[i]);
                    rx_len++;
                }
                if (rx_total != -1 && rx_len >= rx_total)
                    break;
            } else {
                break;
            }
        }
        printf("\n");
    }

err1:
    serial_close(serial);
    serial_free(serial);
err2:
    lockfile_remove();
    return 0;
}