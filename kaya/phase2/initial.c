#include "../h/const.h"
#include "../h/types.h"

/* phase 2 global variables */
int proccessCount;
int softBlockCount;
pcb_PTR currentProcess;
pcd_PTR readyQ;
/* the order of devices on the array is as follows: 
	[timer, 8 disk devices, 8 tape devices, 8 network adapters, 8 printer devices, 8 terminal devices] */
int semaphoreArray[SEMCOUNT]; 
/* cpu_t processStartTime */

void main(){
	/* populate 4 new areas in low memory:
		set stack pointer to last page of physcial memory (RAMTOP) (same for all)
		set PC to appropriate function for handler of each
		set status reg: VM off, Interrupts masked, Supervisor mode on (same for all) */

	devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR; /* rambase address */
	memaddr ramtop = busRegArea->rambase + busRegArea->ramsize; /* ramtop address */

	unsigned int statusRegValue = ALLOFF | VMOFF | KERNELON | INTERRUPTSMASKED; /* represents VM off, interrupts masked, and kernel mode on */

	/* initialize syscall new area */
	/* set PC to syscallHandler */
	state_t* syscallNew = (state_t*) SYSCALLNEWAREA;
	syscallNew->s_pc = (memaddr) SyscallHandler; /* in exceptions.c */
	syscallNew->s_t9 = (memaddr) SyscallHandler; /* always set t9 to be the same as pc */
	/* set stack pointer to last page of physcial memory (RAMTOP) */
	syscallNew->s_sp = ramtop;
	/* set status reg: VM off, Interrupts masked, Supervisor mode on */
	syscallNew->s_status = statusRegValue;

	/* initialize programTrap new area */
	/* set PC */
	state_t* programTrapNew = (state_t*) PROGRAMTRAPNEWAREA;
	programTrapNew->s_pc = (memaddr) ProgramTrapHandler; /* in exceptions.c */
	programTrapNew->s_t9 = (memaddr) ProgramTrapHandler; 
	/* set stack pointer */
	programTrapNew->s_sp = ramtop;
	/* set status */
	programTrapNew->s_status = statusRegValue;

	/* initialize TLBManagement new area */
	/* set PC */
	state_t* TLBMgmtNew = (state_t*) TLBMANAGEMENTNEWAREA;
	TLBMgmtNew->s_pc = (memaddr) TLBManagementHandler; /* in exceptions.c */
	TLBMgmtNew->s_t9 = (memaddr) TLBManagementHandler; 
	/* set stack pointer */
	TLBMgmtNew->s_sp = ramtop;
	/* set status */
	TLBMgmtNew->s_status = statusRegValue;

	/* initialize interrupt new area */
	/* set PC */
	state_t* interruptNew = (state_t*) INTERRUPTNEWAREA;
	interruptNew->s_pc = (memaddr) InterruptHandler; /* in interrupts.c */
	interruptNew->s_t9 = (memaddr) InterruptHandler;
	/* set stack pointer */
	interruptNew->s_sp = ramtop;
	/* set status */
	interruptNew->s_status = statusRegValue;

	/* initialize data structures */
	initPcbs();
	initASL();

	/* initialize globals */
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	readyQ = mkEmptyProcQ();

	/* initialize nucleus maintained semaphores */
	for(int i=0; i < SEMCOUNT; i++){
		semaphoreArray[i] = 0;
	}

	pcb_PTR initialProc = allocPcb(); /* initial process */
	/* initialize its state: 
		set stack pointer to penultimate page of physical memory
		set PC to p2test
		set status: VM off, Interrupts enabled/unmasked, Supervisor mode on */
	initialProc->p_s->s_sp = ramtop - PAGESIZE;
	initialProc->p_s->s_pc = p2test; /* change based on name */
	initialProc->p_s->s_status = ALLOFF | VMOFF | INTERRUPTSUNMASKED | KERNELON;

	processCount++;

	insertProcQ(&readyQ, initialProc);

	scheduler();
}
