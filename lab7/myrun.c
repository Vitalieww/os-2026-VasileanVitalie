#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s program [args...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Execută programul primit în linia de comandă */
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[myrun] %s exited with status %d\n", argv[1], WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[myrun] %s killed by signal %d\n", argv[1], WTERMSIG(status));
    }

    return 0;
}
