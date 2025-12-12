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
  int e_status;

  if(argint(0, &e_status) < 0){
    return -1;
  }
  e_status = e_status << 8; 
  exit(e_status);
  return 0;  
}

int
sys_wait(void)
{
  int *status;

  if(argptr(0, (void**)&status, sizeof(int)) < 0){
    return -1;
  }

  return wait(status);
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
sys_getprio(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;

  return getprio(pid);
}


int
sys_setprio(void)
{
  int pid;
  int aux;

  if(argint(0, &pid) < 0)
    return -1;

  if(argint(1, &aux) < 0)
    return -1;
  
  unsigned int prio = (unsigned int) aux;

  return setprio(pid, prio);
}



int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  addr = myproc()->sz; 

  if(n<0)
  {
    if (growproc(n) < 0)
      return -1;
  }
  else
  {
    myproc()->sz += n; 
  }
  

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

int 
sys_date(void)
{
  struct rtcdate *r;

  if(argptr(0, (void **)&r, sizeof(struct rtcdate)) < 0){
    return -1;
  }
  cmostime(r);

  return 0;
}