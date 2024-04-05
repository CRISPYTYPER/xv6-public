#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int ret = fork(); // fork() is called, create a new process
  if (ret < 0) {
    // fork failed; exit
    printf(1, "fork failed\n");
    exit();
  } else if (ret == 0) {
    // child process
    while(1) {
      printf(1, "Child\n");
      yield();
    }

  } else {
    // parent process
    while(1) {
      printf(1, "Parent\n");
      yield();
    }
    wait();
  }
  exit();
}