#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  int gpid = getgpid();
  printf(1, "My student id is 2019040591\n");

  // Exception handling
  if(pid < 0){
    printf(1, "getpid failed\n");
    exit();
  } else {
    printf(1, "My pid is %d\n", pid);
  }

  if(gpid < 0){
    printf(1, "getgpid failed\n");
    exit();
  } else {
    printf(1, "My gpid is %d\n", gpid);
  }

  exit();
}