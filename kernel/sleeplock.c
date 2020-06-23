// Sleeping locks

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->holder = 0;
}

void
addsleeplock(struct sleeplock *lk)
{
  struct proc *p = myproc();
  int i;
  acquire(&p->lock);
  for (i = 0; i < NSLEEPLOCK; i++){
    if(p->sleeplocks[i]==0){
      p->sleeplocks[i] = lk;
      release(&p->lock);
      return;
    }
  }
  release(&p->lock);
}

void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  while (lk->locked) {
    if(lk->holder->prio_effective > myproc()->prio_effective){
      lk->holder->prio_effective = myproc()->prio_effective;
    }
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->holder = myproc();
  addsleeplock(lk);
  release(&lk->lk);
}

void removesleeplock(struct sleeplock *lk){
  struct proc *p = myproc();
  acquire(&p->lock);
  int i;
  for (i = 0; i < NSLEEPLOCK; i++){
    if(p->sleeplocks[i]==lk){
      p->sleeplocks[i] = 0;
      release(&p->lock);
      return;
    }
  }
  release(&p->lock);
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  myproc()->prio_effective=myproc()->prio_base;
  lk->locked = 0;
  lk->holder = 0;
  removesleeplock(lk);
  wakeup(lk);
  release(&lk->lk);
  yield();
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked && (lk->holder == myproc());
  release(&lk->lk);
  return r;
}



