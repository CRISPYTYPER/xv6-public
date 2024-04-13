#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  // add it for lab03
  SETGATE(idt[128], 1, SEG_KCODE<<3, vectors[128], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  // add for lab 03
  case 128:
    cprintf("user interrupt 128 called!\n");
    exit();
    break;
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      // For Project 02
      myproc()->usedtq += 1;  // Increment timequantum of running process by 1
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // // Force process to give up CPU on clock tick.
  // // If interrupts were on while locks held, would need to check nlock.
  // if(myproc() && myproc()->state == RUNNING &&
  //    tf->trapno == T_IRQ0+IRQ_TIMER)
  //   yield();

  // For Project 02

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER){
      acquire(&mlfq.lock);
      if(myproc()->usedtq == mlfq.timequantums[myproc()->qnum]){ // 시간 다 사용
      release(&mlfq.lock);
        switch(myproc()->qnum){  // the queue number where myproc() is in
          // 스케쥴러에서 골라질 때 이미 레디큐에서 빼놔서 집어넣기만 하면 됨.
          case 0: // L0 큐에서 실행되던 프로세스인 경우
            if(myproc()->pid % 2 == 1){ // pid가 홀수인 프로세스들은 L1 큐로 내려가고, tq 초기화
              putintoL1(myproc());
            } else{  // pid가 짝수인 프로세스들은 L2 큐로 내려가고, tq 초기화
              putintoL2(myproc());
            }
            break;
          case 1:  // L1 큐에서 실행되던 프로세스인 경우
            putintoL3(myproc());
            break;
          case 2:  // L2 큐에서 실행되던 프로세스인 경우
            putintoL3(myproc());
            break;
          case 3: // L3 큐에서 실행되던 프로세스인 경우
            // TODO: priority를 1 감소시키고, timequantum초기화. mlfq 프로세스들 재배열해야함
            if(myproc()->priority > 0)  // priority가 0에서는 더 감소하지 않음
              myproc()->priority--;
            myproc()->usedtq = 0;
            // 스케쥴러에서 골라졌을때(RUNNING)이미 해당 레디큐에서 빠졌었음. 그래서 여기서 다시 넣어줘야함.
            putintoL3(myproc());

          default:
            panic("invalid pid value!!(trap.c)");
          }
        }
    
      yield();  // give up the CPU held to myproc()
     }

  // Priority boosting
  // Global tick이 100ticks가 될 때 마다 모든 프로세스들을 L0 큐로 재조정하기 & 모든 프로세스들의 timequantum 초기화하기
  if(tf->trapno == T_IRQ0+IRQ_TIMER && ticks % 100 == 0){
    acquire(&mlfq.lock);
    // 먼저 L0에 있는 프로세스들의 tq 0으로 초기화
    int i;
    uint curprocidxL0 = mlfq.curprocidx[0]; // L0큐의 첫 프로세스가 있는 인덱스
    for(i = 0; i < mlfq.qlengths[0]; i++){
      mlfq.queues[0][(curprocidxL0+i)%NPROC]->usedtq = 0; // curprocidxL0부터 프로세스 개수만큼 usedtq 초기화
    }
    // L1 차례
    uint curprocidxL1 = mlfq.curprocidx[1]; // L1큐의 첫 프로세스가 있는 인덱스
    uint lengthL1 = mlfq.qlengths[1]; // L1 큐의 프로세스의 개수
    for(i = 0; i < lengthL1; i++){
      mlfq.queues[1][(curprocidxL1+i)%NPROC]->usedtq = 0; // curprocidxL1부터 프로세스 개수만큼 usedtq 초기화
      mlfq.queues[1][(curprocidxL1+i)%NPROC]->qnum = 0; // L0 소속으로 변경
      mlfq.queues[0][(curprocidxL0+mlfq.qlengths[0])%NPROC] = mlfq.queues[1][(curprocidxL1+i)%NPROC];  // L1의 프로세스들을 L0큐의 마지막(curprocidx에서 시작해서 논리상 제일 오른쪽) 프로세스 뒤에 붙이기
      mlfq.qlengths[0]++;
      mlfq.qlengths[1]--;
    }
    if(mlfq.qlengths[1] != 0)  // for debugging
      panic("Size of L1 not 0 when gloabl boosting!");
    // L2 차례
    uint curprocidxL2 = mlfq.curprocidx[2]; // L2프로세스의 첫 프로세스가 있는 인덱스
    uint lengthL2 = mlfq.qlengths[2];  // L2 큐의 프로세스의 개수
    for(i = 0; i < lengthL2; i++){
      mlfq.queues[2][(curprocidxL2+i)%NPROC]->usedtq = 0; // curprocidxL2부터 프로세스 개수만큼 usedtq 초기화
      mlfq.queues[2][(curprocidxL2+i)%NPROC]->qnum = 0; // L0 소속으로 변경
      mlfq.queues[0][(curprocidxL0+mlfq.qlengths[0])%NPROC] = mlfq.queues[2][(curprocidxL2+i)%NPROC];  // L2 프로세스들을 L0큐의 마지막(curprocidx에서 시작해서 논리상 제일 오른쪽) 프로세스 뒤에 붙이기
      mlfq.qlengths[0]++;
      mlfq.qlengths[2]--;
    }
    if(mlfq.qlengths[2] != 0)  // for debugging
      panic("Size of L2 not 0 when gloabl boosting!");
    // L3 차례
    // 어차피 L3는 첫 인덱스가 0임
    uint lengthL3 = mlfq.qlengths[3];  // L3 큐의 프로세스의 개수
    for(i = 0; i < lengthL3; i++){
      mlfq.queues[3][i]->usedtq = 0; // curprocidxL3부터 프로세스 개수만큼 usedtq 초기화
      mlfq.queues[3][i]->qnum = 0; // L0 소속으로 변경
      mlfq.queues[0][(curprocidxL0+mlfq.qlengths[0])%NPROC] = mlfq.queues[3][i];  // L3 프로세스들을 L0큐의 마지막(curprocidx에서 시작해서 논리상 제일 오른쪽) 프로세스 뒤에 붙이기
      mlfq.qlengths[0]++;
      mlfq.qlengths[3]--;
    }
    if(mlfq.qlengths[3] != 0)  // for debugging
      panic("Size of L3 not 0 when gloabl boosting!");
    release(&mlfq.lock);
  } 

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
