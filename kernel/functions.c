#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock randlock;
static uint state = 1;
static const uint a = 1664525;
static const uint b = 1013904223;

void
lcg_srand(uint seed)
{
    acquire(&randlock);
    state = seed;
    release(&randlock);
}

uint
lcg_rand(void)
{
    uint v;
    acquire(&randlock);
    state = a * state + b;
    v = state;
    release(&randlock);
    return v;
}

void
setgid(int gid)
{
    struct proc *p = myproc();
    acquire(&p->lock);
    p->gid = gid;
    release(&p->lock);
}

int
getgid(void)
{
    struct proc *p = myproc();
    int gid;
    acquire(&p->lock);
    gid = p->gid;
    release(&p->lock);
    return gid;
}
