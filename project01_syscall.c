#include "types.h"
#include "defs.h"
#include "proc.h"

// System call for returning parent PID
int
getppid(void)
{
  return myproc()->parent->pid;
}

