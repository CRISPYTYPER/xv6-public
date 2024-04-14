#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// lab 04 practice 2
int
sys_print_ticks_pid_name(void) {
  acquire(&tickslock);
  cprintf("ticks = %d, pid = %d, name = %s\n", ticks, myproc()->pid, myproc()->name);
  release(&tickslock);
  return 0;
}

// Project 02
// Required system calls
int
sys_yield(void) {
  /**
   * 자신이 점유한 cpu를 앙보합니다.
  */
  yield();
  return 0;
}

int 
sys_getlev(void)
{
  /**
   * 프로세스가 속한 큐의 레벨을 반환합니다.
   * MoQ에 속한 프로세스인 경우 99를 반환합니다.
  */
  if(myproc()->state == RUNNABLE){ // RUNNABLE 한 상태에 있는 프로세스에만 유효함
    if(myproc()->ismoq == 0){  // 일반 mlfq에 들어있는 프로세스이면
      return myproc()->qnum;
    }else{  // 만약 moq에 들어있는 프로세스이면
      return 99;
    }
  }else{
    return -1;
  }
}

int
sys_setpriority(void)
{
  /**
   * arguments: (int pid, int priority)
   * 특정 pid를 가지는 프로세스의 priority를 설정합니다.
   * priority 설정에 성공한 경우 0을 반환합니다.
   * 주어진 pid를 가진 프로세스가 존재하지 않는 경우 -1을 반환합니다.
   * priority가 0 이상 10 이하의 정수가 아닌 경우 -2를 반환합니다.
  */

  int pid, priority;

  // argint fetches the nth 32-bit system call argument
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &priority) < 0)
    return -1;
  return setprioriy(pid, priority);  // proc.c
}

int
setmonopoly(int pid, int password)
{
  /**
   * 특정 pid를 가진 프로세스를 MoQ로 이동합니다. 인자로 독점 자격을 증명할 암호(자신의 학번)을 받습니다.
   * 암호가 일치할 경우, MoQ를 반환합니다.
   * 존재하지 않는 포르세스의 pid인 경우 -1을 반환합니다.
   * 암호가 일치하지 않는 경우 -2를 반환합니다.
   * 이미 MoQ에 존재하는 프로세스인 경우 -3을 반환합니다.
   * 자기 자신을 MoQ로 이동시키려 하는 경우 -4를 반환합니다.
  */
}

void
monopolize(void)
{
  /**
   * MoQ의 프로세스가 CPU를 독점하여 사용하도록 설정합니다.
  */
}

void
unmonopolize(void)
{
  /**
   * 독점적 스케줄링을 중지하고 기존의 MLFQ part로 돌아갑니다.
  */
}