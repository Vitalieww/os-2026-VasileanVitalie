#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p2c[2]; // parent -> child: [0] read, [1] write
    int c2p[2]; // child -> parent: [0] read, [1] write
    char byte = 'x';

    if (pipe(p2c) < 0 || pipe(c2p) < 0) {
        printf("pipe failed\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        // Child process
        close(p2c[1]); // Close parent-write end
        close(c2p[0]); // Close child-read end

        // Read ping from parent
        read(p2c[0], &byte, 1);
        printf("%d: received ping\n", getpid());
        close(p2c[0]);

        // Write pong back to parent
        write(c2p[1], &byte, 1);
        close(c2p[1]);

        exit(0);
    } else {
        // Parent process
        close(p2c[0]); // Close parent-read end
        close(c2p[1]); // Close child-write end

        // Send ping to child
        write(p2c[1], &byte, 1);
        close(p2c[1]);

        // Wait to read pong back
        read(c2p[0], &byte, 1);
        printf("%d: received pong\n", getpid());
        close(c2p[0]);

        wait(0);
        exit(0);
    }
}
