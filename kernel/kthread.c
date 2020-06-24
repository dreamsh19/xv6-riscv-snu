//--------------------------------------------------------------------
//
//  4190.307: Operating Systems (Spring 2020)
//
//  PA#6: Kernel Threads
//
//  June 2, 2020
//
//  Jin-Soo Kim
//  Systems Software and Architecture Laboratory
//  Department of Computer Science and Engineering
//  Seoul National University
//
//--------------------------------------------------------------------

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"

#ifdef SNU

extern pagetable_t kernel_pagetable;

void ret()
{
  struct proc *p = myproc();
  release(&p->lock);
  p->fn(p->arg);
  kthread_exit();
}

static struct proc *
allocthread(void)
{
  struct proc *t;

  for (t = proc; t < &proc[NPROC]; t++)
  {
    acquire(&t->lock);
    if (t->state == UNUSED)
    {
      goto found;
    }
    else
    {
      release(&t->lock);
    }
  }
  return 0;

found:
  t->pid = allocpid();

  t->pagetable = kernel_pagetable;

  memset(&t->context, 0, sizeof t->context);
  t->context.ra = (uint64)ret;
  t->context.sp = t->kstack + PGSIZE;
  t->rr_scheduled = 0;
  
  return t;
}

int kthread_create(const char *name, int prio, void (*fn)(void *), void *arg)
{
  struct proc *t;
  if ((t = allocthread()) == 0)
    return -1;

  safestrcpy(t->name, name, sizeof(name));

  t->fn = fn;
  t->arg = arg;

  t->prio_base = prio;
  t->prio_effective = prio;
  t->state = RUNNABLE;


  if (prio < kthread_get_prio()){
      release(&t->lock);
      kthread_yield();
  }else{
    release(&t->lock);
  }

  return t->pid;
}

void freethread(struct proc *p)
{
  int i;
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  // p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->rr_scheduled = 0;
  for (i = 0; i < NSLEEPLOCK; i++)
    p->sleeplocks[i] = 0;
  p->fn = 0;
  p->arg = 0;
  p->state = UNUSED;
}

void kthread_exit(void)
{
  struct proc *t = myproc();
  acquire(&t->lock);
  freethread(t);
  sched();
}

void kthread_yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

void kthread_set_prio(int newprio)
{
  struct proc *t = myproc();
  acquire(&t->lock);
  if (t->prio_base != t->prio_effective){
    // donation
    t->prio_base = newprio;
    if(newprio < t->prio_effective){
      t->prio_effective = newprio;
    }
    release(&t->lock);
  }else{
    // no donation
    t->prio_base = newprio;
    t->prio_effective = newprio;
    release(&t->lock);    
    kthread_yield();

  }
}

int kthread_get_prio(void)
{
  return myproc()->prio_effective;
}
#endif
