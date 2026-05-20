// Israeli lock: FIFO queue of waiters with gid-based favoritism.

struct israeli_lock {
  struct spinlock lock;           // protects fields below
  int active;                     // 1 if created, 0 if free/destroyed
  int favoritism;                 // 0..100 (percent)
  struct proc *owner;             // current holder, or 0 if free
  struct proc *queue[NISWAIT];    // FIFO of waiting processes
  int qcount;                     // number of entries in queue
};
