#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid;
  int status = 0;

  if (argc < 2) {
    // Default test run when invoked without arguments: run echo hello from the child
    char *echo_argv[] = { "echo", "hello", "from", "the", "child", 0 };
    pid = fork();
    if (pid < 0) {
      fprintf(2, "run: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      printf("child: pid=%d\n", getpid());
      exec("echo", echo_argv);
      fprintf(2, "run: exec echo failed\n");
      exit(1);
    }
    wait(&status);
    printf("parent: child exited with status %d\n", status);
    exit(0);
  }

  // General run invocation with user-specified command: run <command> [args...]
  pid = fork();
  if (pid < 0) {
    fprintf(2, "run: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    printf("child: pid=%d\n", getpid());
    exec(argv[1], &argv[1]);
    fprintf(2, "run: exec %s failed\n", argv[1]);
    exit(1);
  }

  wait(&status);
  printf("parent: child exited with status %d\n", status);
  exit(0);
}

