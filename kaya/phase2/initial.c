#include "../h/const.h"
#include "../h/types.h"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"


/* phase 2 global variables */
int processCount;
int softBlockCount;
pcb_PTR currentProcess;
pcb_PTR readyQ;
/* the order of devices on the array is as follows: 
	[8 disk devices, 8 tape devices, 8 network adapters, 8 printer devices, 8 terminal devices, timer] */
int semaphoreArray[SEMCOUNT]; 
cpu_t processStartTime;

HIDDEN void initializeNewArea(state_PTR memArea, memaddr handlerName, memaddr sp, unsigned int status);

void main(){
	/* populate 4 new areas in low memory:
		set stack pointer to last page of physcial memory (RAMTOP) (same for all)
		set PC to appropriate function for handler of each
		set status reg: VM off, Interrupts masked, Supervisor mode on (same for all) */

	devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR; /* rambase address */
	memaddr ramtop = busRegArea->rambase + busRegArea->ramsize; /* ramtop address */

	unsigned int statusRegValue = ALLOFF | VMOFF | KERNELON | INTERRUPTSMASKED; /* represents VM off, interrupts masked, and kernel mode on */

	initializeNewArea(state_PTR SYSCALLNEWAREA, memaddr SyscallHandler, memaddr ramtop, unsigned int statusRegValue);
	initializeNewArea(state_PTR PROGRAMTRAPNEWAREA, memaddr ProgramTrapHandler, memaddr ramtop, unsigned int statusRegValue);
	initializeNewArea(state_PTR TLBMANAGEMENTNEWAREA, memaddr TLBManagementHandler, memaddr ramtop, unsigned int statusRegValue);
	initializeNewArea(state_PTR INTERRUPTNEWAREA, memaddr InterruptHandler, memaddr ramtop, unsigned int statusRegValue);

	/* initialize data structures */
	initPcbs();
	initASL();

	/* initialize globals */
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	readyQ = mkEmptyProcQ();
	processStartTime = NULL;

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

HIDDEN void initializeNewArea(state_PTR memArea, memaddr handlerName, memaddr sp, unsigned int status)
{
	/* initialize the new area */
	/* set PC to handler function */
	state_PTR newMemArea = (state_PTR) memArea;
	newMemArea->s_pc = (memaddr) handlerName; /* in exceptions.c */
	newMemArea->s_t9 = (memaddr) handlerName; /* always set t9 to be the same as pc */
	/* set stack pointer to last page of physcial memory (RAMTOP) */
	newMemArea->s_sp = sp;
	/* set status reg: VM off, Interrupts masked, Supervisor mode on */
	newMemArea->s_status = status; /* setStatus?? */
}