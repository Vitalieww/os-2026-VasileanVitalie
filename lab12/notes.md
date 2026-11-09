# Laboratory Work No. 12: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** Producer, Consumer and the Other Tools (Track A - Linux)

---

## Part 1: Producer and Consumer (`prodcons.c`)

### Program Overview
- Source file: `prodcons.c`
- Implements Dijkstra's classic bounded-buffer problem using a ring buffer of `SIZE = 4` slots transferring `ITEMS = 8` values.
- Three synchronization primitives:
  - `sem_t empty`: counting semaphore initialized to `SIZE` ($4$), tracking available free slots.
  - `sem_t full`: counting semaphore initialized to $0$, tracking occupied slots ready for consumption.
  - `pthread_mutex_t lock`: mutual exclusion lock protecting the buffer indices (`in`, `out`) and buffer modifications.

### Compilation & Execution Trace
```bash
$ gcc -Wall -pthread -o prodcons prodcons.c
$ ./prodcons
put 1 (in buffer: 1)
put 2 (in buffer: 2)
put 3 (in buffer: 3)
put 4 (in buffer: 4)
      take 1 (in buffer: 3)
put 5 (in buffer: 4)
      take 2 (in buffer: 3)
put 6 (in buffer: 4)
      take 3 (in buffer: 3)
put 7 (in buffer: 4)
      take 4 (in buffer: 3)
put 8 (in buffer: 4)
      take 5 (in buffer: 3)
      take 6 (in buffer: 2)
      take 7 (in buffer: 1)
      take 8 (in buffer: 0)
```

### Observe Questions & Answers
1. **In `prodcons.c`, which call makes the producer stop when the buffer is full?**
   - The call to **`sem_wait(&empty);`** in the producer function.
   - When the producer fills all 4 slots before the slow consumer finishes reading, `empty` reaches 0. Any subsequent call to `sem_wait(&empty)` puts the producer thread to sleep until the consumer consumes an item and executes `sem_post(&empty)`.

2. **When the program starts, does the consumer run first or does it wait? Which line makes it do that?**
   - The consumer thread **waits** (blocks).
   - The line responsible is **`sem_wait(&full);`** in `consumer()`.
   - Because `full` is initialized to 0 (`sem_init(&full, 0, 0)`), any attempt by the consumer to decrement `full` forces it into a blocked state until the producer writes an item and executes `sem_post(&full)`.

3. **What would happen if the producer locked the mutex BEFORE calling `sem_wait(&empty)`?**
   - **Catastrophic Deadlock:**
     - If the buffer becomes full (`empty == 0`), the producer acquires `lock` first, then calls `sem_wait(&empty)`.
     - Because `empty` is 0, the producer blocks while still holding `lock`.
     - The consumer attempts to consume an item to free space, but cannot enter its critical section because it blocks at `pthread_mutex_lock(&lock)`.
     - The producer waits for the consumer to increment `empty`, while the consumer waits for the producer to release `lock`. Both threads remain permanently frozen in circular deadlock.

---

## Part 2: Condition Variable (`condvar.c`)

### Program Overview
- Source file: `condvar.c`
- Implements thread signaling using POSIX condition variables (`pthread_cond_t changed`), an associated mutex (`pthread_mutex_t lock`), and an explicit state variable (`int ready = 0`).
- Reader checks `ready`; if false, calls `pthread_cond_wait(&changed, &lock)`. Writer populates message, sets `ready = 1`, and signals via `pthread_cond_signal(&changed)`.

### Compilation & Execution Trace
```bash
$ gcc -Wall -pthread -o condvar condvar.c
$ ./condvar
B: not ready, I sleep
A: message written
B: I read "hello"
```

### Observe Questions & Answers
1. **Which variable remembers that the message is ready?**
   - The integer variable **`int ready`**.
   - Unlike semaphores, condition variables are purely stateless wait-queues and have no internal counter or memory. If a signal is sent when no thread is waiting on a condition variable, the signal is lost. Therefore, persistent state must always be maintained in an explicit application variable (`ready`).

2. **In `reader()`, replace `while (!ready)` with `if (!ready)`. Does it still work?**
   - **Yes**, in this specific single-producer single-consumer program, replacing `while` with `if` still functions properly because there is only one reader thread and no competing consumers to change `ready` back to 0.

3. **Why does POSIX recommend `while` instead of `if`?**
   - **Spurious Wakeups:** Under Linux and POSIX specifications, `pthread_cond_wait()` can legally wake up even in the absence of a signal (e.g., due to kernel-level signal interrupts or futex implementation specifics).
   - **Multiple Competing Consumers (Mesa Semantics):** If multiple reader threads are sleeping and one writer signals `pthread_cond_broadcast` or multiple threads wake up, another thread could acquire the mutex first, consume the state, and reset `ready = 0`. With `while`, the woken thread re-evaluates `!ready` and goes back to sleep safely; with `if`, it would erroneously proceed with invalid state.

---

## Part 3: Readers and Writers (`readers.c`)

### Program Overview
- Source file: `readers.c`
- Solves the Readers-Writers problem (favoring readers) using two semaphores:
  - `sem_t room`: binary semaphore initialized to 1, controlling exclusive access to the shared resource.
  - `sem_t rlock`: binary semaphore initialized to 1, protecting atomic access to the integer counter `readers`.

### Compilation & Execution Trace
```bash
$ gcc -Wall -pthread -o readers readers.c
$ ./readers
writer 1 writes 10 (alone)
reader 1 reads 10 (readers inside: 1)
reader 2 reads 10 (readers inside: 2)
reader 3 reads 10 (readers inside: 3)
writer 2 writes 20 (alone)
```

### Observe Questions & Answers
1. **Which two lines allow multiple readers to read at the same time?**
   - **Line 22–23:** `if (++readers == 1) sem_wait(&room);`
   - **Line 30–31:** `if (--readers == 0) sem_post(&room);`
   - **Explanation:** Only the first reader that enters acquires `room`, effectively locking out all writers. Subsequent readers increment `readers` and proceed straight to reading without waiting on `room`. Similarly, only the last exiting reader releases `room` back to waiting writers.

2. **Why does the `readers` counter need its own lock `rlock`?**
   - Modifying `readers` (`++readers` and `--readers`) involves non-atomic read-modify-write CPU cycles.
   - Without `rlock`, concurrent readers entering or exiting simultaneously could create a race condition on `readers`. A corrupted counter could cause two readers to both believe they are the first reader (double `sem_wait`), or prevent the last reader from signaling `sem_post(&room)`, permanently locking out writers.

3. **What happens if readers keep arriving one after another, with no gap?**
   - **Writer Starvation:**
     - Because new readers can enter freely as long as `readers > 0`, continuous reader arrivals keep `readers >= 1` indefinitely.
     - The `room` semaphore is never released, causing waiting writers to be starved of CPU time indefinitely.

---

## Part 4: Deadlock (`deadlock.c`)

### Program Overview
- Source file: `deadlock.c`
- Demonstrates Coffman's circular wait condition when two threads acquire shared resources in inverted order:
  - Thread `one`: locks `A`, sleeps 1s, then requests `B`.
  - Buggy Thread `two`: locks `B`, sleeps 1s, then requests `A`.
- Fixed version: Both threads adhere to a strict lock hierarchy (lock `A` first, then lock `B`).

### Execution & Timeout Detection
```bash
$ gcc -Wall -pthread -o deadlock_bug deadlock_bug.c
$ timeout 4 ./deadlock_bug
one: I have A, I want B
two: I have B, I want A
$ echo "exit code: $?"
exit code: 124

$ gcc -Wall -pthread -o deadlock deadlock.c
$ timeout 4 ./deadlock
one: I have A, I want B
one: I have both
two: I have A, I want B
two: I have both
done
$ echo "exit code: $?"
exit code: 0
```

### Observe Questions & Answers
1. **Draw the wait circle on paper: thread one holds ..., wants ...; thread two holds ..., wants ...:**
   - **Resource Allocation Graph (RAG) Wait Cycle:**
     - Thread `one` **holds Lock A**, and **requests Lock B**.
     - Thread `two` **holds Lock B**, and **requests Lock A**.
     $$\text{Thread 1} \xrightarrow{\text{holds}} \text{Mutex A} \xleftarrow{\text{wants}} \text{Thread 2} \xrightarrow{\text{holds}} \text{Mutex B} \xleftarrow{\text{wants}} \text{Thread 1}$$
     - This creates a closed directed dependency cycle: $\text{Thread 1} \to \text{Mutex B} \to \text{Thread 2} \to \text{Mutex A} \to \text{Thread 1}$.

2. **If you delete `sleep(1)`, the program might occasionally finish without hanging. Why is that worse than hanging every time?**
   - Deleting `sleep(1)` converts a deterministic bug into a non-deterministic **Heisenbug** (intermittent concurrency defect).
   - When a program hangs deterministically (every single run), it is detected immediately in test suites and fixed before release.
   - When a race/deadlock is intermittent (e.g., occurring 1 in 10,000 runs depending on CPU timing), tests usually pass, allowing the catastrophic defect to leak into production systems where it causes sporadic, untraceable system lockups.

3. **Fix the program: change only thread two so it takes the locks in the same order as thread one (A, then B). Run it again. Does it finish now?**
   - **Yes**, the fixed program runs cleanly to completion in 2 seconds and prints `done` with exit code `0`.
   - By enforcing a strict global lock ordering ($A \prec B$), circular wait is mathematically broken (Coffman condition violated), preventing deadlock entirely.
