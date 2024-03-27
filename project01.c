#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  int ppid = getppid();
  printf(1, "Current PID : %d\n", pid);
  printf(1, "Parent PID : %d\n", ppid);
}