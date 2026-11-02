# Laboratory Work No. 2: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Meet the OS You Will Build (xv6)

---

## Part 1: Setup and Build Environment

### Observations
1. **Cross-Compilation Toolchain:**
   - The development workstation runs an x86_64 host environment (Ubuntu on WSL2), whereas the target operating system (xv6) is compiled for 64-bit RISC-V (`rv64gc`).
   - Consequently, the cross-compiler `riscv64-linux-gnu-gcc` is strictly required to translate source code into RISC-V machine instructions and object files.
   - Hardware execution is virtualized using the `qemu-system-riscv64` emulator configured with the `virt` platform machine model.

2. **Boot Sequence & SMP Startup:**
   - When the kernel boots, it initializes CPU hardware thread 0 (hart 0).
   - In symmetric multiprocessing (SMP) mode, xv6 configures multiple hardware threads (`hart 1 starting`, `hart 2 starting`), synchronizes them, and proceeds to initialize kernel paging, the slab-like process table, and virtio disk drivers before launching the first user process (`init: starting sh`).

---

## Part 2: Using xv6 as the Unix It Is

### Observe Questions & Answers
1. **Name three programs that xv6 ships with:**
   - From running `ls` in the root filesystem, xv6 includes standard Unix utilities such as:
     1. `ls` — lists directory contents, file types, inode numbers, and file sizes.
     2. `cat` — concatenates files or standard input to standard output.
     3. `echo` — prints its arguments to standard output.
     *(Other programs include `grep`, `wc`, `mkdir`, `rm`, `sh`, and `kill`)*.

2. **What two OS features are required for a pipeline like `ls | grep c` to work?**
   - **Isolated Concurrent Processes:** The operating system must support multitasking and address space isolation (`fork()`). The shell must be able to spawn two independent processes that execute concurrently.
   - **Inter-Process Communication (IPC) via Pipes / File Descriptors:** The kernel must provide a shared communication channel (`pipe()`) backed by a bounded ring buffer in kernel memory, along with file descriptor redirection (`dup()` / `close()`) that attaches the standard output of the producer process (`ls`) to the pipe write end and standard input of the consumer process (`grep`) to the pipe read end.

3. **How does the xv6 shell compare to the Linux shell explored in Lab 1?**
   - **Conceptual Parity:** Both shells follow the classic Unix philosophy: programs are simple, composable tools communicating via text streams. Both support foreground process execution (`fork` + `exec`), descriptor redirection (`<`, `>`), and pipelines (`|`).
   - **Simplicity vs. Feature Set:** The xv6 shell (`user/sh.c`) is minimalist (under 500 lines of C), purposefully designed for pedagogical clarity. It lacks advanced features found in modern Linux shells (such as Bash or Zsh): no environment variables (`$PATH`, `$HOME`), no tab completion, no command history or readline support, no job control (`Ctrl+Z`, `bg`, `fg`), no aliases, and no complex shell scripting syntax (control flow loops, functions, conditionals).

---

## Part 3: Reading the Source Code

### Observe Questions & Answers
1. **What system calls does `user/cat.c` use?**
   - `cat.c` relies on five fundamental system calls declared in `user/user.h`:
     - `open(path, mode)` — opens the file specified by command-line argument.
     - `read(fd, buf, sizeof(buf))` — reads blocks of up to 512 bytes from the file descriptor into a static buffer.
     - `write(1, buf, n)` — writes the read buffer contents to standard output (file descriptor 1) or prints error messages to standard error (file descriptor 2).
     - `close(fd)` — releases the open file descriptor.
     - `exit(status)` — terminates process execution and returns an exit code.

2. **Where in the kernel is `sys_read` implemented?**
   - The implementation of `sys_read` is located in:
     - **File:** `kernel/sysfile.c`
     - **Location:** Line 69:
       ```c
       uint64
       sys_read(void)
       {
         struct file *f;
         int n;
         uint64 p;

         argfd(0, 0, &f);
         argaddr(1, &p);
         argint(2, &n);
         return fileread(f, p, n);
       }
       ```
   - It validates and unmarshals user arguments (`argfd`, `argaddr`, `argint`) from the trapframe registers and delegates the read operation to the internal kernel routine `fileread()`.

3. **What is the difference between the code in `kernel/` and `user/`?**
   - **Privilege Mode:** Code in `kernel/` runs in RISC-V **Supervisor Mode** (privileged mode). Code in `user/` runs in RISC-V **User Mode** (unprivileged mode).
   - **Hardware Access & Memory Safety:**
     - The `kernel/` code has direct access to physical memory, CPU control and status registers (CSRs), page table root registers (`satp`), timer interrupts, and memory-mapped I/O (MMIO) hardware devices (such as virtio disk and UART).
     - The `user/` code operates within its own isolated virtual address space defined by user page tables. It cannot access physical hardware, other processes' memory, or kernel memory.
   - **Control Flow Transition:** User programs cannot jump directly into kernel code. They must initiate a system call by loading the syscall number into register `a7` and triggering an `ecall` trap instruction (via assembly stubs in `user/usys.S`). The CPU hardware then switches to supervisor mode, saves registers, and vectors to the kernel trap handler.

---

## Part 4: Implementing the Sleep Utility

### Implementation Summary
- **Source File:** `user/sleep.c`
- **Logic:**
  1. Validates that exactly one argument is passed (`argc == 2`). If missing or invalid, it prints `usage: sleep <ticks>\n` to file descriptor 2 (stderr) and calls `exit(1)`.
  2. Converts the string argument to an integer using `atoi(argv[1])`.
  3. Invokes the `pause()` system call (xv6 timer tick delay) with the specified tick count.
  4. Returns `exit(0)` upon completion.
- **Makefile Integration:**
  - Added `$U/_sleep\` to `UPROGS` in `Makefile`.

### Verification & Testing
```bash
$ sleep 20
# Pauses for approximately 2 seconds (20 timer ticks) and returns to prompt

$ sleep
usage: sleep <ticks>
```

