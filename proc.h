// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  // For Project 02
  int qnum;                    // Indicates which queue is it in (-1 if not in MLFQ)
  uint usedtq;                 // Used time quantum
  uint priority;               // Priority for L3 queue
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

// For Project02
// Multi Level Feedback Queue structure
typedef struct {
  struct spinlock lock;
  struct proc* queues[NQUEUE][NPROC];  // Process queues (4 levels)
  uint qlengths[NQUEUE];               // Count of processes in each queue
  uint timequantums[NQUEUE];            // Time quantum of each process
  uint curprocidx[NQUEUE];              // Current idx in each queue where to run(increment by 1 as proceeds. not used in L3)

  struct proc* moq[NPROC];  // MOQ
  uint moqlength;           // length of moq
} mlfq_t;

extern mlfq_t mlfq;

// to use these in trap.c
void putintoL0(struct proc *p);
void putintoL1(struct proc *p);
void putintoL2(struct proc *p);
void putintoL3(struct proc *p);