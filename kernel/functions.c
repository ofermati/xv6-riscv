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

// Per-team scores for the relay-race tournament (Task 2).
static struct spinlock teamlock;
static int team_scores[NTEAMS];

void
teaminit(void)
{
    initlock(&teamlock, "team");
    for(int i = 0; i < NTEAMS; i++)
        team_scores[i] = 0;
}

// Increment team's score by one and return the new value.
// Returns -1 if team id is out of range.
int
team_score_inc(int team)
{
    if(team < 0 || team >= NTEAMS)
        return -1;
    acquire(&teamlock);
    int v = ++team_scores[team];
    release(&teamlock);
    return v;
}

int
team_score_get(int team)
{
    if(team < 0 || team >= NTEAMS)
        return -1;
    acquire(&teamlock);
    int v = team_scores[team];
    release(&teamlock);
    return v;
}

void
team_score_reset(void)
{
    acquire(&teamlock);
    for(int i = 0; i < NTEAMS; i++)
        team_scores[i] = 0;
    release(&teamlock);
}
