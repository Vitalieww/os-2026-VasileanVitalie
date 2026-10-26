#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int p[2];
    if (pipe(p) < 0) {
        perror("pipe");
        exit(1);
    }

    /* Copil 1: ls -l /etc */
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(1);
    }

    if (pid1 == 0) {
        dup2(p[1], STDOUT_FILENO);
        close(p[0]);
        close(p[1]);
        execlp("ls", "ls", "-l", "/etc", (char *)NULL);
        perror("execlp ls");
        exit(127);
    }

    /* Copil 2: wc -l */
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(1);
    }

    if (pid2 == 0) {
        dup2(p[0], STDIN_FILENO);
        close(p[0]);
        close(p[1]);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("execlp wc");
        exit(127);
    }

    /* Părintele închide ambele capete pentru a permite trimiterea EOF către wc */
    close(p[0]);
    close(p[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
