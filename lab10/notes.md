# Laboratory Work No. 10: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Add Your Own System Call: trace (xv6)

---

## Part 1: Architecture of an xv6 System Call

### Required Files and Modifications
1. **`kernel/syscall.h`:**
   - Defined numeric system call constant:
     ```c
     #define SYS_trace 23
     ```
2. **`user/user.h`:**
   - Added userland prototype declaration:
     ```c
     int trace(int);
     ```
3. **`user/usys.pl`:**
   - Added generator entry to emit the RISC-V assembly stub:
     ```perl
     entry("trace");
     ```
   - Emits RISC-V instructions:
     ```assembly
     .global trace
     trace:
      li a7, SYS_trace
      ecall
      ret
     ```
4. **`user/trace.c`:**
   - User program that parses the trace mask argument (`atoi(argv[1])`), installs it via `trace()`, and executes the child command (`exec(argv[2], argv + 2)`).
5. **`Makefile`:**
   - Registered `$U/_trace\` under the `UPROGS` variable.

---

## Part 2: Kernel Implementation

### Key Modifications
1. **`kernel/proc.h`:**
   - Added `int trace_mask;` field to `struct proc`.
2. **`kernel/sysproc.c`:**
   - Implemented `sys_trace()`:
     ```c
     uint64
     sys_trace(void)
     {
       int mask;
       argint(0, &mask);
       myproc()->trace_mask = mask;
       return 0;
     }
     ```
3. **`kernel/syscall.c`:**
   - Added `extern uint64 sys_trace(void);`
   - Added `[SYS_trace] = sys_trace` to dispatch table `syscalls[]`.
   - Defined `syscallnames[]` array mapping syscall numbers to string literals.
   - Added interceptor logging inside `syscall()`:
     ```c
     p->trapframe->a0 = syscalls[num]();
     if ((p->trace_mask >> num) & 1) {
       printk("%d: syscall %s -> %ld\n",
              p->pid, syscallnames[num], p->trapframe->a0);
     }
     ```
4. **`kernel/proc.c`:**
   - Added inheritance of trace mask in `fork()`:
     ```c
     np->trace_mask = p->trace_mask;
     ```

---

## Part 3: Verification & Answers to Observe Questions

### 1. Check 2 Verification: `trace 32 grep hello README`
```bash
$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 965
3: syscall read -> 453
3: syscall read -> 0
```
- Sum of read bytes: $1023 + 965 + 453 = 2441$ bytes.
- This matches the exact file size of `README` in the xv6 root filesystem (`ls` reports `2441`).
- The final `read -> 0` indicates End of File (EOF).

### 2. Observe Questions & Answers
1. **Why does `trace 32 grep hello README` only display `read` system calls?**
   - The value $32$ equals $2^5$ (bit 5 is set: `0b0000000000100000`).
   - In `kernel/syscall.h`, `SYS_read` has identifier `5`.
   - The bitwise condition `(p->trace_mask >> num) & 1` evaluates to `1` exclusively when `num == 5`. Other calls executed by `grep` (such as `open` which is 15, and `close` which is 21) have their corresponding bits set to `0` in the mask and are filtered out.

2. **Which line in Part 2 allows printing syscalls for distinct PIDs in the `forkforkfork` test?**
   - The line in `kernel/proc.c` inside `fork()`:
     ```c
     np->trace_mask = p->trace_mask;
     ```
   - This copies the parent process's `trace_mask` to the newly allocated child process (`np`). Consequently, all child and descendant processes inherit the tracing mask and print their intercepted system calls with their own unique PIDs.

3. **Why does the trace output appear interleaved between "hi" and the newline in `trace 65536 echo hi`?**
   - Value $65536 = 2^{16}$, setting bit 16 for `SYS_write`.
   - The xv6 `echo` utility (`user/echo.c`) issues two distinct `write()` calls:
     1. First call: writes the string argument (`write(1, "hi", 2)`). As soon as the system call returns in `kernel/syscall.c`, the kernel outputs the trace line:
        ```text
        hi66: syscall write -> 2
        ```
     2. Second call: writes the trailing newline character (`write(1, "\n", 1)`), which prints the newline and triggers the second trace line:
        ```text
        66: syscall write -> 1
        ```

4. **Calculate the bitmask required to trace exclusively `open` (15) and `close` (21):**
   - $2^{15} = 32768$
   - $2^{21} = 2097152$
   - $\text{Mask} = 2^{15} + 2^{21} = 32768 + 2097152 = \mathbf{2129920}$
   - Verification command:
     ```bash
     $ trace 2129920 grep hello README
     3: syscall open -> 3
     3: syscall close -> 0
     ```

