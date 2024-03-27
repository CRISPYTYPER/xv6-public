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
  return myproc()->parent->pid;
}

