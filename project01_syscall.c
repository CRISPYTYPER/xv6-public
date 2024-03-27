#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"

// System call for returning parent PID
int
getppid(void)
{
  struct proc *currentProcess = myproc();  // Fetch the current process structure.
  // Check if the current process or its parent is NULL.
  if(currentProcess == 0 || currentProcess->parent == 0)
    return -1;
  return myproc()->parent->pid;
}

// Wrapper for getppid()
int
sys_getppid(void)
{
  int ppid;
  ppid = getppid();
  return ppid;
}


