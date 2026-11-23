# Lab 17: Threads Share Memory, and Race

## Observe 1: Non-zero initial counter in Thread 2
Thread 2 printing a non-zero `counter` at the start proves that both threads share the exact same virtual address space (the global data/BSS segment) rather than having isolated copies of global variables.

## Observe 2: Why the final counter is less than 2000000 and varies each run
`counter++` is not an atomic operation; at the machine-instruction level it executes in three steps:
1. **Read (load):** Load `counter` from shared memory into a CPU register.
2. **Add:** Increment the value in that register by 1.
3. **Write (store):** Store the updated register value back into `counter` in memory.

When the OS scheduler interleaves threads between these steps, two threads read the same stale value, increment it independently, and the second store overwrites the first—losing an update. Because thread scheduling and core execution timing are non-deterministic, the exact number of lost updates changes on every run.

## Observe 3: Single idea to stop lost updates
Protect the `counter++` critical section with a mutual exclusion lock (mutex) or a hardware atomic increment instruction so only one thread can read, add, and write `counter` at a time.
