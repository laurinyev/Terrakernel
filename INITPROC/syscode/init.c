#include <sys/syscalls.h>

void _start() {
    const char msg[] = "Hello\n";
    int fd;

    fd = sys_open("output.txt", 577, 0644);
    if (fd >= 0) {
        sys_write(fd, msg, sizeof(msg) - 1);

		sys_write(0, "Wrote to file\n\0", 15);

        sys_close(fd);
    }

    while (1) {
    }
}
