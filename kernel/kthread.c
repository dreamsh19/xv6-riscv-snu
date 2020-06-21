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
extern struct proc proc[];

void (*FN)(void *);
void *ARG;

void ret()
{
  release(&myproc()->lock);
  FN(ARG);
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
  return t;
}

int kthread_create(const char *name, int prio, void (*fn)(void *), void *arg)
{
  struct proc *t;
  if ((t = allocthread()) == 0)
    return -1;

  safestrcpy(t->name, name, sizeof(name));
  // t->context.s0 =(uint64)arg;
  // t->context.ra =(uint64)fn;

  FN = fn;
  ARG = arg;

  t->prio_base = prio;
  t->prio_effective = prio;
  t->state = RUNNABLE;

  release(&t->lock);

  if (prio < kthread_get_prio())
    kthread_yield();

  return t->pid;
}

void freethread(struct proc *p)
{
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  // p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
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
  t->prio_base = newprio;
  if (newprio < t->prio_effective)
    t->prio_effective = newprio;
}

int kthread_get_prio(void)
{
  return myproc()->prio_effective;
}
#endif
