# Laboratory Work No. 8: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Processes Talking Through Pipes (xv6)

---

## Part 1: RUN, Inside xv6

### Observe Questions & Answers
1. **What three system calls did you use to run a program in xv6? How do they correspond to the calls you used on Linux in Lab 7?**
   - **xv6 System Calls:** `fork()`, `exec()`, and `wait()`.
   - **Linux Comparison (Lab 7):** On Linux, we used `fork()`, `execvp()` (or `execlp()`), and `waitpid()`.
   - **Key Differences:**
     - In xv6, `exec(char *path, char **argv)` requires a direct file path and null-terminated argument array; it does not parse or search an environment `$PATH` variable.
     - In xv6, `wait(int *status)` blocks until *any* child exits and directly writes the child's integer exit status to the integer pointer. On Linux, `waitpid(pid, &status, options)` allows targeting a specific PID, non-blocking polling (`WNOHANG`), and encodes exit status and terminating signals into bit fields requiring POSIX macros (`WIFEXITED`, `WEXITSTATUS`, `WTERMSIG`) to decode.

2. **In the `echo` case, who prints "hello from the child"? The child process, or the `echo` program?**
   - The **`echo` program** prints "hello from the child", executing inside the address space of the **child process**.
   - When `fork()` is called, a duplicate child process is created. The child then invokes `exec("echo", echo_argv)`. The `exec` system call replaces the existing memory space, text segment, and stack of the child with the `echo` binary image. The CPU resumes execution at `echo`'s `main()`, which writes its argument strings to file descriptor 1 (`stdout`) before calling `exit(0)`.

3. **When running `nosuchprog`, why did the parent report exit status 1 in xv6, whereas on Linux in Lab 7 it reported 127?**
   - **xv6 Behavior:** In xv6, when `exec()` fails (because `nosuchprog` does not exist in the filesystem), it returns control to the calling code with return value `-1`. In `run.c`, the child checks for this failure and executes `exit(1)`. The kernel records this exit code directly, and the parent's `wait(&status)` retrieves status `1`.
   - **Linux POSIX Convention (Lab 7):** Under Linux and standard POSIX shell conventions, exit status `127` is specifically designated to denote "command not found" (and `126` for "command invoked cannot execute"). In Lab 7, `myrun.c` explicitly followed this standard by calling `exit(127)` when `execvp()` failed. xv6 does not have elaborate status codes and uses `1` for general execution failure.

---

## Part 2: PINGPONG

### Observe Questions & Answers
1. **Why are two pipes needed for bidirectional communication?**
   - Pipes in Unix and xv6 are strictly **unidirectional**: data written to descriptor `pipe[1]` can only be read sequentially from descriptor `pipe[0]`.
   - If a single pipe were used for both parent and child:
     - The parent could write a byte and then immediately read from the same pipe, accidentally reading back its own byte before the child gets scheduled.
     - Race conditions and unpredictable deadlocks would occur.
   - Therefore, two separate pipes are strictly necessary:
     - `p2c[2]` (Parent-to-Child): Parent writes to `p2c[1]`, Child reads from `p2c[0]`.
     - `c2p[2]` (Child-to-Parent): Child writes to `c2p[1]`, Parent reads from `c2p[0]`.

2. **Which system call makes the parent wait for the child before printing its pong?**
   - The blocking system call **`read(c2p[0], &byte, 1)`** makes the parent wait.
   - In Unix/xv6 pipe semantics, invoking `read()` on an empty pipe whose write descriptors remain open will put the calling process into sleep state until at least one byte of data is written or until all write descriptors are closed. Hence, the parent sleeps in `read()` until the child processes the ping and issues `write(c2p[1], &byte, 1)`.

---

## Part 3: PRIMES

### Summary & Design
- Doug McIlroy's concurrent prime sieve pipeline creates a chain of processes connected via pipes.
- The root process feeds numbers 2 through 35 into the first pipe.
- Each stage reads the first incoming integer $p$, prints it as prime, and forks a right-neighbor process connected by a new pipe.
- The current stage reads remaining integers from its left pipe: if $n \% p \neq 0$, it writes $n$ to the right pipe.
- **File Descriptor Hygiene:** Every stage must close its unused read/write ends to prevent leaking descriptors (`NOFILE` limit in xv6 is 16 per process) and ensure the end-of-file (EOF) token propagates cleanly through the pipeline so processes terminate in cascade.

---

## Part 4: XARGS

### Summary & Design
- `xargs` reads standard input byte-by-byte, assembling arguments delimited by newlines.
- For every line of input, `xargs` appends the parsed arguments to the command-line arguments and dispatches the command via `fork()`, `exec()`, and `wait(0)`.
- Correctly integrates with xv6 pipeline utilities (e.g., `echo hello | xargs echo`).

