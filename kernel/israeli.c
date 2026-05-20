#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "israeli.h"

struct israeli_lock ilocks[NISLOCK];

void
israeliinit(void)
{
  for(int i = 0; i < NISLOCK; i++){
    initlock(&ilocks[i].lock, "israeli");
    ilocks[i].active = 0;
    ilocks[i].favoritism = 0;
    ilocks[i].owner = 0;
    ilocks[i].qcount = 0;
  }
}

// Remove queue[idx] and shift the rest down.
// Caller must hold L->lock.
static void
queue_remove(struct israeli_lock *L, int idx)
{
  for(int i = idx + 1; i < L->qcount; i++)
    L->queue[i-1] = L->queue[i];
  L->qcount--;
}

int
israeli_create(int favoritism)
{
  if(favoritism < 0 || favoritism > 100)
    return -1;

  for(int i = 0; i < NISLOCK; i++){
    acquire(&ilocks[i].lock);
    if(!ilocks[i].active){
      ilocks[i].active = 1;
      ilocks[i].favoritism = favoritism;
      ilocks[i].owner = 0;
      ilocks[i].qcount = 0;
      release(&ilocks[i].lock);
      return i;
    }
    release(&ilocks[i].lock);
  }
  return -1;
}

int
israeli_acquire(int lock_id)
{
  if(lock_id < 0 || lock_id >= NISLOCK)
    return -1;

  struct israeli_lock *L = &ilocks[lock_id];
  struct proc *p = myproc();

  acquire(&L->lock);
  if(!L->active){
    release(&L->lock);
    return -1;
  }
  if(L->qcount >= NISWAIT){
    release(&L->lock);
    return -1;
  }

  // Enqueue ourselves at the tail.
  L->queue[L->qcount++] = p;

  // Wait until we are the owner. If the lock is free and we are at the head
  // of the queue, claim it directly (handles the very first acquire when no
  // one will ever wake us).
  while(L->owner != p){
    if(L->owner == 0 && L->queue[0] == p){
      L->owner = p;
      queue_remove(L, 0);
      break;
    }
    sleep(p, &L->lock);
    if(!L->active){
      release(&L->lock);
      return -1;
    }
  }

  release(&L->lock);
  return 0;
}

int
israeli_release(int lock_id)
{
  if(lock_id < 0 || lock_id >= NISLOCK)
    return -1;

  struct israeli_lock *L = &ilocks[lock_id];
  struct proc *p = myproc();

  acquire(&L->lock);
  if(!L->active || L->owner != p){
    release(&L->lock);
    return -1;
  }

  int G = p->gid;
  L->owner = 0;

  if(L->qcount > 0){
    int idx = 0; // default: FIFO

    // Look for the earliest waiter with gid == G.
    int fav_idx = -1;
    for(int i = 0; i < L->qcount; i++){
      if(L->queue[i]->gid == G){
        fav_idx = i;
        break;
      }
    }

    // With probability favoritism%, pick the favored waiter.
    if(fav_idx >= 0 && (int)(lcg_rand() % 100) < L->favoritism)
      idx = fav_idx;

    struct proc *next = L->queue[idx];
    queue_remove(L, idx);
    L->owner = next;
    wakeup(next);
  }

  release(&L->lock);
  return 0;
}

int
israeli_destroy(int lock_id)
{
  if(lock_id < 0 || lock_id >= NISLOCK)
    return -1;

  struct israeli_lock *L = &ilocks[lock_id];

  acquire(&L->lock);
  if(!L->active){
    release(&L->lock);
    return -1;
  }

  L->active = 0;
  L->owner = 0;
  // Wake every waiter so they observe !active and return -1.
  for(int i = 0; i < L->qcount; i++)
    wakeup(L->queue[i]);
  L->qcount = 0;

  release(&L->lock);
  return 0;
}
