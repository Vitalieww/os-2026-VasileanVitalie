#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }

        /* Redirecționează stdout către out.txt */
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(1);
        }
        close(fd);

        execlp("ls", "ls", "-l", (char *)NULL);
        perror("execlp");
        exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return 0;
}
