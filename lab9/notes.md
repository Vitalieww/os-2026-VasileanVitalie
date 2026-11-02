# Laboratory Work No. 9: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Watching System Calls with strace

---

## Part 1: Watching `ls -l` with strace

### Execution Command
```bash
strace -f -e trace=openat,read,write,close ls -l
```

### Observe Questions & Answers
1. **Which system call does `ls` use to display directory contents?**
   - The `ls` utility uses the **`write()`** system call to output formatted text to the terminal:
     ```text
     write(1, "total 28\ndrwxrwxr-x ...", 385) = 385
     ```
   - Before writing, `ls` uses `openat()` to open directory file descriptors and reads directory entries (via `getdents64` and metadata via `newfstatat`).

2. **What is the file descriptor number passed to that call?**
   - The file descriptor passed is **`1`**, corresponding to standard output (**`stdout`**).

---

## Part 2: Direct Kernel Calls (`syscalls.c`)

### Program Overview
- Source file: `syscalls.c`
- Interacts directly with the kernel using low-level POSIX primitives:
  - `open("note.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644)`
  - `write(fd, msg, strlen(msg))`
  - `close(fd)`
  - `open("note.txt", O_RDONLY)`
  - `read(fd, buf, sizeof(buf) - 1)`
  - `close(fd)`

### Execution Trace
```bash
$ gcc -Wall -Werror -o syscalls syscalls.c
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
1. **Why is the descriptor returned by the first `open()` call 3, and not 0, 1, or 2?**
   - Under POSIX standards, every newly spawned process inherits three standard streams pre-allocated by convention:
     - `0` $\rightarrow$ Standard Input (`stdin`)
     - `1` $\rightarrow$ Standard Output (`stdout`)
     - `2` $\rightarrow$ Standard Error (`stderr`)
   - The Linux kernel file descriptor allocation policy mandates selecting the **lowest available non-negative integer** in the process descriptor table (`struct files_struct`). Because slots 0, 1, and 2 are occupied at process startup, the first successful invocation of `open()` is assigned index **`3`**.
   - When descriptor 3 is subsequently closed via `close(3)`, its slot becomes free. Therefore, the second `open("note.txt", O_RDONLY)` call also receives index **`3`**.

2. **How does `printf()` relate to `write()`?**
   - `printf()` is a high-level user-space library function provided by the C standard library (`glibc` / `musl`). It formats string representations into a user-space memory buffer.
   - When the user buffer is flushed (e.g., encountering a newline `\n` in line-buffered mode or reaching buffer capacity), `printf()` invokes the kernel system call `write(1, buffer, length)`.
   - In contrast, invoking `write()` directly bypasses library-level buffering and triggers an immediate trap transition into kernel mode.

