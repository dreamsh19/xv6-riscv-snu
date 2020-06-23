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
    
    struct proc *lock_holder=lk->holder;
    struct sleeplock *sl;

level:
    if(lock_holder->prio_effective > myproc()->prio_effective){
      lock_holder->prio_effective = myproc()->prio_effective;
    }

    if(lock_holder->state == SLEEPING){
      sl=(struct sleeplock *) lock_holder->chan;
      lock_holder = sl->holder;
      goto level;
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
  int i;
  for (i = 0; i < NSLEEPLOCK; i++){
    if(p->sleeplocks[i]==lk){
      p->sleeplocks[i] = 0;
      return;
    }
  }
}

void
set_prio_effective(){
  struct proc *my = myproc();
  struct proc *p;
  struct sleeplock *lk;
  int minPrio = my->prio_base;
  for (int i = 0; i < NSLEEPLOCK; i++){
    if((lk = my->sleeplocks[i])){
      for (p = proc; p < &proc[NPROC]; p++){
        if (p == myproc()) continue;
        acquire(&p->lock);
        if(p->chan == lk && p->state == SLEEPING && p->prio_effective < minPrio){
          minPrio = p->prio_effective;
        }
        release(&p->lock);

      }
    }
  }
  my->prio_effective = minPrio;
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->holder = 0;
  acquire(&myproc()->lock);
  removesleeplock(lk);
  set_prio_effective();
  release(&myproc()->lock);
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



