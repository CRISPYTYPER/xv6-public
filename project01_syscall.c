#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"

// Kernel function for returning parent PID
int
getppid(void)
{
  struct proc *currentProcess = myproc();  // Fetch the current process structure.
  // Check if the current process or its parent is NULL.
  if(currentProcess == 0 || currentProcess->parent == 0)
    return -1;
  return currentProcess->parent->pid;
}

// Kernel function for returning grandparent PID
int
getgpid(void)
{
  struct proc* currentProcess = myproc();  // Fetch the current process structure.
  // Null check
  if(currentProcess == 0 || currentProcess->parent == 0 || currentProcess->parent->parent == 0)
    return -1;
  return currentProcess->parent->parent->pid;
}

// System call for getppid()
int
sys_getppid(void)
{
  return getppid();
}

// System call for getgpid()
int
sys_getgpid(void)
{
  return getgpid();
}


