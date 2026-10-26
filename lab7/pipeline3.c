#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int p1[2], p2[2];
    if (pipe(p1) < 0 || pipe(p2) < 0) {
        perror("pipe");
        exit(1);
    }

    /* Child 1: ls /usr/bin */
    pid_t pid1 = fork();
    if (pid1 < 0) { perror("fork 1"); exit(1); }
    if (pid1 == 0) {
        dup2(p1[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("ls", "ls", "/usr/bin", (char *)NULL);
        perror("execlp ls");
        exit(127);
    }

    /* Child 2: grep x */
    pid_t pid2 = fork();
    if (pid2 < 0) { perror("fork 2"); exit(1); }
    if (pid2 == 0) {
        dup2(p1[0], STDIN_FILENO);
        dup2(p2[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("grep", "grep", "x", (char *)NULL);
        perror("execlp grep");
        exit(127);
    }

    /* Child 3: wc -l */
    pid_t pid3 = fork();
    if (pid3 < 0) { perror("fork 3"); exit(1); }
    if (pid3 == 0) {
        dup2(p2[0], STDIN_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("execlp wc");
        exit(127);
    }

    /* Parent closes all ends */
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}
