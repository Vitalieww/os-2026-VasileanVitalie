# Laboratory Work No. 13: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Semaphores in Your Own Kernel (Track B - xv6)

---

## Part 1: Kernel Semaphore Implementation (`kernel/sem.c`)

### System Architecture
- Implemented counting semaphores inside the xv6 operating system kernel.
- A static table of 8 semaphores (`NSEM = 8`) resides in kernel memory:
  ```c
  struct sem {
    struct spinlock lock;
    int value;
  };
  static struct sem sems[NSEM];
  ```
- Three core system calls were registered with unique IDs:
  - `sem_init(id, value)` $\rightarrow$ Syscall 24
  - `sem_down(id)` $\rightarrow$ Syscall 25
  - `sem_up(id)` $\rightarrow$ Syscall 26

### System Call Handlers
1. **`sys_sem_init`:** Validates semaphore index ($0 \le id < 8$) and non-negative initial value ($value \ge 0$). Acquires spinlock, assigns value, releases spinlock.
2. **`sys_sem_down`:** Enforces the xv6 sleep protocol to avoid missed wakeups:
   ```c
   acquire(&s->lock);
   while (s->value == 0) {
     sleep_prepare(s);
     release(&s->lock);
     sleep();
     acquire(&s->lock);

     if (killed(myproc())) {
       release(&s->lock);
       return -1;
     }
   }
   s->value--;
   release(&s->lock);
   return 0;
   ```
3. **`sys_sem_up`:** Increments semaphore value atomically and awakens any sleeping processes queued on address `s`:
   ```c
   acquire(&s->lock);
   s->value++;
   wakeup(s);
   release(&s->lock);
   return 0;
   ```

---

## Part 2: Verification with `semtest` (`user/semtest.c`)

### Test Coverage
- **Test 1 (Order):** Semaphore 0 starts at 0. Child process calls `sem_down(0)`. Parent sleeps (`pause(10)`), prints "parent: done, I let the child go", and calls `sem_up(0)`. Proves child does not execute until signaled.
- **Test 2 (Ping-Pong):** Two semaphores (`sem 1 = 1`, `sem 2 = 0`). Parent and child alternate execution in lockstep (`ping 1`, `pong 1`, `ping 2`, `pong 2`, `ping 3`, `pong 3`).

### Execution Trace
```text
$ semtest
parent: done, I let the child go
child: I ran after the parent
ping 1
pong 1
ping 2
pong 2
ping 3
pong 3
semtest: OK
```

---

## Part 3: Console Output Interleaving with `mix` (`user/mix.c`)

### Execution Without Synchronization (`mix`)
```text
$ mix
AAbAAAbAbbbAbAbbAbbAbbAbAAAbAb
AbAbAbbAbAbbAbAbAbAbAbAbAbAbAb
AbAbAbAbAbAbAbAbAbAbAbAbAbAbAb
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```
- Characters from parent ('A') and child ('b') interleave arbitrarily within lines.

### Execution With Semaphore Protection (`mix lock`)
```text
$ mix lock
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
```
- Each 30-character line prints as an uninterrupted atomic block.

---

## Observe Questions & Answers

1. **In the first run (without `lock`), why do the letters mix?**
   - In `mix.c`, each process prints its line character-by-character using individual single-byte system calls: `write(1, p, 1)`.
   - In a multi-core or preemptive environment (xv6 configured with 3 CPUs via QEMU `-smp 3`), both processes run in parallel on separate hardware cores. Each character transmission crosses into the kernel console driver independently. Because no locking protects the sequence of 30 writes, individual character outputs from both processes interleave on the shared console device.

2. **Which two lines in `mix.c` make one line a critical section?**
   - **Line 10:** `sem_down(3);` immediately before entering the character printing loop.
   - **Line 14:** `sem_up(3);` immediately after printing the line and trailing newline.
   - Together, these two calls convert the entire line-printing loop into an atomic critical section protected by binary semaphore index 3.

3. **In `sem_down`, why `while (s->value == 0)` and not `if`?**
   - When a process calls `wakeup(s)` in `sem_up`, **all** processes sleeping on the semaphore channel `s` are marked `RUNNABLE`.
   - When these processes wake up, they must re-acquire `s->lock` one at a time. The first waking process to acquire the lock decrements `s->value` from 1 to 0 and proceeds.
   - If subsequent awakened processes used `if`, they would bypass the check, decrement `s->value` below 0, and violate the semaphore invariant.
   - Using `while` ensures every woken process re-checks whether a unit is actually available before continuing; if not, it safely re-enters `sleep()`.

4. **In `sem_down`, why `release(&s->lock)` before `sleep()`?**
   - If the calling process entered `sleep()` while continuing to hold `s->lock`, no other process could ever acquire `s->lock`.
   - Specifically, any partner process attempting to execute `sem_up(id)` would block indefinitely attempting to acquire `s->lock` at line 71.
   - As a consequence, `wakeup(s)` could never be called, and the sleeping process could never be awakened. The entire system would enter an unrecoverable kernel deadlock. Releasing `s->lock` inside the sleep window (after `sleep_prepare(s)` has registered the wait channel) ensures other CPUs can access the semaphore to signal `sem_up`.
