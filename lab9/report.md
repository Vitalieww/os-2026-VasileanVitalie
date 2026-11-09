# Laboratory Work No. 9: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Watching System Calls with strace (Track A - Linux)

---

## Part 1: See the Calls of a Real Command (`ls -l`)

### Execution Command
```bash
$ strace -f -e trace=openat,read,write,close ls -l
```

### Trace Output Excerpt
```text
openat(AT_FDCWD, ".", O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY) = 3
write(1, "total 28\ndrwxrwxr-x ...", 385) = 385
close(3)                                = 0
close(1)                                = 0
close(2)                                = 0
```

### Observe Questions & Answers
1. **Which system call does `ls` use to display directory contents?**
   - The `ls` utility uses the **`write()`** system call to output formatted text to the terminal:
     ```text
     write(1, "total 28\ndrwxrwxr-x ...", 385) = 385
     ```
2. **To which descriptor does it write?**
   - The file descriptor passed is **`1`**, which corresponds to standard output (**`stdout`**).

---

## Part 2: A Program That Calls the Kernel Directly (`syscalls.c`)

### Program Overview
- Source file: `syscalls.c`
- Calls kernel primitives directly without user-space file buffering:
  - `open("note.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644)`
  - `write(fd, msg, strlen(msg))`
  - `close(fd)`
  - `open("note.txt", O_RDONLY)`
  - `read(fd, buf, sizeof(buf) - 1)`
  - `close(fd)`

### Compilation & Execution Trace
```bash
$ gcc -Wall -Wextra -o syscalls syscalls.c
$ ./syscalls
open returned descriptor 3
write sent 14 bytes
read got 14 bytes: hello, kernel

$ strace -e trace=openat,write,read,close ./syscalls
openat(AT_FDCWD, "note.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
write(1, "open returned descriptor 3\n", 27) = 27
write(3, "hello, kernel\n", 14)           = 14
write(1, "write sent 14 bytes\n", 20)      = 20
close(3)                                = 0
openat(AT_FDCWD, "note.txt", O_RDONLY)  = 3
read(3, "hello, kernel\n", 63)          = 14
write(1, "read got 14 bytes: hello, kernel\n", 33) = 33
close(3)                                = 0
```

### Observe Questions & Answers
1. **The descriptor returned by the first `openat` is 3, not 0, 1 or 2. Why are those three already taken?**
   - Under POSIX standards, every newly initialized process inherits three open standard file descriptors:
     - `0` $\rightarrow$ Standard Input (`stdin`)
     - `1` $\rightarrow$ Standard Output (`stdout`)
     - `2` $\rightarrow$ Standard Error (`stderr`)
   - The Linux kernel file descriptor allocation policy mandates selecting the lowest available non-negative integer in the process's file descriptor table (`struct files_struct`). Because slots 0, 1, and 2 are pre-occupied, the first call to `open()` is allocated index **`3`**. When descriptor 3 is closed and reopened, index **`3`** is recycled.

2. **How does `printf()` relate to `write()`?**
   - `printf()` is a high-level user-space library function provided by the C standard library (`glibc`). It formats characters into a user-space memory buffer.
   - When the user-space buffer is flushed (e.g., on a newline `\n` in line-buffered mode or when full), `printf()` invokes the kernel system call `write(1, buffer, length)`.
   - In contrast, invoking `write()` directly bypasses library-level buffering and triggers an immediate trap transition into kernel mode.

---

## Part 3: Count One Kind of Call (`strace -c`)

### Execution Command
The `-c` flag tells `strace` to run the target program, tally all intercepted system calls, measure execution time, and output a summary table at process termination:
```bash
$ strace -c cat syscalls.c > /dev/null
```

### Summary Output Table
```text
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 25.17    0.000037          18         2           read
 19.05    0.000028           7         4           mmap
 14.29    0.000021          10         2           openat
 12.24    0.000018           6         3           close
  9.52    0.000014           7         2           newfstatat
  6.80    0.000010          10         1           write
  4.76    0.000007           7         1           munmap
  4.08    0.000006           6         1           brk
  4.08    0.000006           6         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.000147                    22           total
```

### Observe Questions & Answers
1. **From the summary, how many `read` calls did `cat` make?**
   - From the summary table, `cat` made **`2`** `read` system calls.

2. **Explain in one sentence why reading a file needs more than one `read`:**
   - A file is read in buffer-sized chunks, and the program cannot know when the end of the file is reached until a subsequent `read()` system call returns `0` bytes (the standard POSIX EOF indicator), which requires an extra call after all data has been ingested.
