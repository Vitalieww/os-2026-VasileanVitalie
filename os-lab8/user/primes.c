#include "kernel/types.h"
#include "user/user.h"

void stage(int left_fd) __attribute__((noreturn));

void
stage(int left_fd)
{
    int p;
    int n;

    // Read the first number; this is our prime
    int bytes = read(left_fd, &p, sizeof(p));
    if (bytes <= 0) {
        close(left_fd);
        exit(0);
    }

    printf("prime %d\n", p);

    int right_pipe[2];
    if (pipe(right_pipe) < 0) {
        close(left_fd);
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        close(left_fd);
        close(right_pipe[0]);
        close(right_pipe[1]);
        exit(1);
    }

    if (pid == 0) {
        // Child stage: read from right_pipe[0]
        close(left_fd);
        close(right_pipe[1]); // Close write end
        stage(right_pipe[0]);
    } else {
        // Current stage: filter numbers to right_pipe[1]
        close(right_pipe[0]); // Close read end

        while (read(left_fd, &n, sizeof(n)) > 0) {
            if (n % p != 0) {
                write(right_pipe[1], &n, sizeof(n));
            }
        }

        // Cleanup and wait for child
        close(left_fd);
        close(right_pipe[1]); // Signals EOF to child
        wait(0);
        exit(0);
    }
}

int
main(int argc, char *argv[])
{
    int p[2];
    if (pipe(p) < 0) {
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        exit(1);
    }

    if (pid == 0) {
        close(p[1]);
        stage(p[0]);
    } else {
        close(p[0]);
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]); // Close write end so first stage sees EOF
        wait(0);
        exit(0);
    }
}
