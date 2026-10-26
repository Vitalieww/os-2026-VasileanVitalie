#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    char *args[MAXARG];
    char buf[512];
    char ch;
    int i;

    if (argc < 2) {
        printf("Usage: xargs <command> [args...]\n");
        exit(1);
    }

    // Copy initial arguments
    for (i = 1; i < argc; i++) {
        args[i - 1] = argv[i];
    }

    int base_argc = argc - 1;
    int pos = 0;

    // Read byte by byte from standard input (fd 0)
    while (read(0, &ch, 1) > 0) {
        if (ch == '\n') {
            buf[pos] = '\0';
            args[base_argc] = buf;
            args[base_argc + 1] = 0; // NULL-terminate argv array

            int pid = fork();
            if (pid < 0) {
                printf("xargs: fork failed\n");
                exit(1);
            }

            if (pid == 0) {
                exec(args[0], args);
                printf("xargs: exec %s failed\n", args[0]);
                exit(1);
            }

            wait(0);
            pos = 0;
        } else {
            if (pos < sizeof(buf) - 1) {
                buf[pos++] = ch;
            }
        }
    }

    // Handle any trailing line without a terminating newline
    if (pos > 0) {
        buf[pos] = '\0';
        args[base_argc] = buf;
        args[base_argc + 1] = 0;

        int pid = fork();
        if (pid == 0) {
            exec(args[0], args);
            exit(1);
        }
        wait(0);
    }

    exit(0);
}
