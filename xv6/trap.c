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

//Declaracion de mappaPages como funcion externa de vm.c
extern int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
//Declaracion de walkpgdir como funcion externa de vm.c
extern pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

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
      exit(1);
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit(1);
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
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
  //hacer un case nuevo para el T_PGFLT y que haga lo que hace el growproc
  case T_PGFLT:
    uint va = rcr2();
    uint pagerr = PGROUNDDOWN(va);
    pde_t * pgfltpde = walkpgdir(myproc()->pgdir, (void *)pagerr, 0);

    if(va >= myproc()->sz)
    {
      if(va >= KERNBASE || va + PGSIZE > KERNBASE)
      {
        cprintf("T_PGFLT accessed a kernel page\n");
        myproc()->killed = 1;
      }
    }
    else if(pgfltpde && *pgfltpde & PTE_P)
    {
      if (!(tf->err & PTE_U))
        panic("kernel had a page fault");

      if (!(*pgfltpde & PTE_U))
        cprintf("Page fault on addr: 0x%x. Tryed to access protected memory.\n", pagerr);

      else
      {
        if (tf->err & PTE_W)
          cprintf("Page fault by error on write access on 0x%x", pagerr);
        else
          cprintf("Page fault by error on read access on 0x%x", pagerr);
      }
      myproc()->killed = 1;
    }
    else
    {
      char *mem;
      mem = kalloc();
      if(mem == 0)
      {
        cprintf("T_PGFLT out of memory\n");
        myproc()->killed = 1;
      }
      else
      {
        memset(mem, 0, PGSIZE);
        if (mappages(myproc()->pgdir, (char *)PGROUNDDOWN(va), PGSIZE, V2P(mem), PTE_W | PTE_U) < 0)
        {
          cprintf("T_PGFLT mapping failed\n");
          kfree(mem);
          myproc()->killed = 1;
        }
      }
    }

    break;










    

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }

    // aqui hay que añadir aqui algo que dice el boletin

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
    exit(1);

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit(1);
}
