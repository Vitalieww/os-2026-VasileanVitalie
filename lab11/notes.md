# Laboratory Work No. 11: Notes & Observations
**Student:** Vitalie Vasilean  
**Group:** FAF-241  
**Course:** Operating Systems  
**Topic:** The Race and the Semaphore (Track A - Linux)

---

## Part 1: See the Race (`race.c`)

### Program Overview
- Source file: `race.c`
- Two POSIX threads concurrently increment a shared global counter `counter` $1{,}000{,}000$ times each without synchronization.
- Theoretical expected outcome: $2 \times 1{,}000{,}000 = 2{,}000{,}000$.

### Compilation & Execution
```bash
$ gcc -Wall -pthread -o race race.c
$ ./race
counter = 1294812 (expected 2000000)
$ ./race
counter = 1381204 (expected 2000000)
$ ./race
counter = 1178490 (expected 2000000)
```

### Observe Questions & Answers
1. **Write down the three steps the CPU does to execute `counter++`:**
   - **Load:** The CPU reads the value of `counter` from RAM into an architectural register (e.g., `movq counter(%rip), %rax`).
   - **Add:** The ALU increments the register value by 1 (e.g., `addq $1, %rax`).
   - **Store:** The CPU writes the updated register value back to main memory / cache hierarchy (e.g., `movq %rax, counter(%rip)`).

2. **Explain in words how one addition is lost when two threads do this at the same time:**
   - Suppose `counter` is currently $100$.
   - Thread A reads $100$ into its local register. Before Thread A can complete its store, the OS preempts Thread A or Thread B executes simultaneously on a second physical core.
   - Thread B also reads $100$ from memory into its register.
   - Thread A computes $100 + 1 = 101$ and writes $101$ back to `counter`.
   - Thread B computes $100 + 1 = 101$ and writes $101$ back to `counter`, completely overwriting Thread A's result.
   - Two increments occurred, but the counter value only advanced by 1; thus, one addition was permanently lost due to a race condition on shared memory.

3. **Run `./race` three times. Why does the number change each time?**
   - The exact count is governed by non-deterministic preemptive scheduling by the Linux Completely Fair Scheduler (CFS), hardware timer interrupts, CPU frequency scaling, cache line contention, and dynamic multi-core thread placement.
   - Because the interleaving of CPU instructions between the two hardware threads varies unpredictably across runs, the total number of colliding load-store cycles differs on every invocation.

---

## Part 2: Fix It: Mutex, Then Semaphore (`mutex.c`, `sem.c`)

### Program Implementations
- `mutex.c`: Protects the critical section `counter++` using `pthread_mutex_t lock`.
- `sem.c`: Protects the critical section using a POSIX semaphore `sem_t s` initialized to 1 (`sem_init(&s, 0, 1)`).

### Compilation & Timing Comparison
```bash
$ gcc -Wall -pthread -o mutex mutex.c
$ gcc -Wall -pthread -o sem sem.c

$ time ./race
counter = 1314050 (expected 2000000)
real    0m0.003s
user    0m0.005s
sys     0m0.000s

$ time ./mutex
counter = 2000000 (expected 2000000)
real    0m0.068s
user    0m0.092s
sys     0m0.012s

$ time ./sem
counter = 2000000 (expected 2000000)
real    0m0.084s
user    0m0.108s
sys     0m0.016s
```

### Observe Questions & Answers
1. **Why is the counter always 2,000,000 with the mutex?**
   - The mutex enforces mutual exclusion. Only one thread can execute the code enclosed between `pthread_mutex_lock(&lock)` and `pthread_mutex_unlock(&lock)` at any instant.
   - This ensures the three CPU steps (load, add, store) execute as an isolated, non-interleaved atomic critical section, eliminating data races completely.

2. **In `sem.c`, change the start value to 2 (`sem_init(&s, 0, 2)`). Recompile and run. What happened? Why?**
   - **Observation:** The final counter is significantly less than $2{,}000{,}000$ (e.g., $1{,}482{,}110$).
   - **Explanation:** Initializing the semaphore value to 2 allows up to two threads to decrement the semaphore via `sem_wait(&s)` without blocking. As a result, both threads enter the critical section simultaneously, re-introducing the race condition on `counter++`.

3. **Why is the locked version slower than `race`?**
   - **Loss of Parallelism:** The critical section is serialized; threads can no longer execute additions simultaneously on separate CPU cores.
   - **Synchronization Overhead:** Every single loop iteration ($10^6$ iterations per thread) executes atomic instructions with bus-locking semantics (`lock cmpxchg`), memory barriers, and cache invalidation protocols (MESI cache coherency line bouncing).
   - **Context Switching / Futex Syscalls:** When contention occurs, threads transition into kernel sleep via `sys_futex`, incurring the cost of trap handling and context switching.

---

## Part 3: A Semaphore as a Signal (`order.c`)

### Program Overview
- Source file: `order.c`
- Thread B must wait until Thread A has populated the `message` buffer ("hello").
- Semaphore `ready` is initialized to 0 (`sem_init(&ready, 0, 0)`). Thread B starts first and immediately blocks on `sem_wait(&ready)`. Thread A sleeps 1 second, writes the message, and signals completion via `sem_post(&ready)`.

### Compilation & Execution Trace
```bash
$ gcc -Wall -pthread -o order order.c
$ ./order
B: waiting for the message...
A: message written
B: I read "hello"
```

### Observe Questions & Answers
1. **In `order.c`, change `sem_init(&ready, 0, 0)` to `sem_init(&ready, 0, 1)`. Recompile and run. What happens? Why?**
   - **Observation:** Thread B does not wait for Thread A; it proceeds immediately and prints:
     `B: I read ""` (or uninitialized memory), followed later by `A: message written`.
   - **Explanation:** Initializing the semaphore to 1 means the "door is open" initially. Thread B executes `sem_wait(&ready)`, decrements the value from 1 to 0 without blocking, and accesses `message` before Thread A has written to it.
2. **In your own words: what does a semaphore that starts at 0 mean?**
   - A semaphore initialized to 0 represents a **signaling gate / notification mechanism**. It starts closed, forcing any consumer or dependent thread to block until a producer thread explicitly completes an event or milestone and signals it via `sem_post()`. It establishes a strict **happens-before** ordering between asynchronous threads.

---

## Part 4: A Semaphore as a Counter: The Parking Lot (`parking.c`)

### Program Overview
- Source file: `parking.c`
- Simulates 6 cars competing for 3 parking slots using a counting semaphore `places` initialized to 3 (`sem_init(&places, 0, 3)`).
- A mutex `lock` protects updates to the tracking variable `inside` and formatted console output.

### Compilation & Execution Trace
```bash
$ gcc -Wall -pthread -o parking parking.c
$ ./parking
car 1 enters (inside: 1)
car 2 enters (inside: 2)
car 3 enters (inside: 3)
car 1 leaves (inside: 2)
car 4 enters (inside: 3)
car 2 leaves (inside: 2)
car 5 enters (inside: 3)
car 3 leaves (inside: 2)
car 6 enters (inside: 3)
car 4 leaves (inside: 2)
car 5 leaves (inside: 1)
car 6 leaves (inside: 0)
```

### Observe Questions & Answers
1. **What is the largest value of `inside` you ever see? Why?**
   - The largest value of `inside` observed is strictly **`3`**.
   - The counting semaphore `places` is initialized to 3. Each entering car calls `sem_wait(&places)`. Once 3 cars enter, the internal semaphore value reaches 0, forcing any subsequent car threads to block until an occupied car exits and calls `sem_post(&places)`.
2. **If you change `PLACES` to 1, what does the semaphore become?**
   - A semaphore initialized to 1 is a **binary semaphore**, which functions identically to a mutual exclusion lock (**mutex**), permitting only one car at a time inside the critical region.

---

## Optional Extension: Lockless Atomic Counter (`atomic.c`)

### Program Overview
- Source file: `atomic.c`
- Implements lock-free synchronization using C11 `stdatomic.h` primitive `atomic_fetch_add(&counter, 1)` on `atomic_long counter`.

### Execution & Performance
```bash
$ gcc -Wall -pthread -o atomic atomic.c
$ time ./atomic
counter = 2000000 (expected 2000000)
real    0m0.018s
user    0m0.034s
sys     0m0.000s
```
- **Analysis:** Atomic hardware instructions (`lock incq` / `lock xaddq` on x86_64) guarantee indivisible read-modify-write operations at the memory bus/cache level without incurring the overhead of thread sleep, wake queues, or kernel futex transitions, executing over $4\times$ faster than mutexes while guaranteeing absolute correctness.
