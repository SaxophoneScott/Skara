/************************************************************* initial.c *********************************************************************
* written by Scott Harrington and Kara Schatz         
*
* Innitial.c is the initial set up for the Kaya operating system.  When called, it sets up all the fields required by Kaya.  
*
* 	Initial.c provides the main instruction, main(), that sets up the Kaya operating system.
*	Initial.c sets up the four main exception state areas, initialize the pcbs and the asl, sets the ready queue,
*	initialize all semaphores to have the value of zero. Then after all of that, allocPcb an initial process, and set it to p2test. After that,
* 	we increment the current process count, and call the scheduler. After the call of scheduler, main is never seen re-entered.																																																											
*																																																															
* 									                                                            	
***********************************************************************************************************************************************/
#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"


/* phase 2 global variables */
int processCount;					/* # processes in the system */
int softBlockCount;					/* # processes blocked */
pcb_PTR currentProcess;				/* the current running process */
cpu_t processStartTime;				/* time the current running process started its turn */
pcb_PTR readyQ;						/* the queue of ready processes */
int semaphoreArray[SEMCOUNT]; 		/* semaphore for each device and the interval timer, organized as follows: 
									[8 disk devices, 
									8 tape devices, 
									8 network adapters, 
									8 printer devices, 
									8 terminal devices (transmit)
									8 terminal devices (receive), 
									interval timer] */


HIDDEN void initializeNewArea(state_PTR memArea, memaddr handlerName, memaddr sp, unsigned int status);
extern void test(); /* method of p2test.c */

void main(){
	int i;															/* loop control variable to initialize sema4s */
	devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR; 		/* rambase address */
	memaddr ramtop = busRegArea->rambase + busRegArea->ramsize; 	/* ramtop address */
	/* status for the 4 new areas, represents VM off, interrupts masked, kernel mode on, and interval timer enabled */
	unsigned int statusRegValue = ALLOFF | INITVMOFF | KERNELON | INTERRUPTSMASKED | TEBITON; 
	pcb_PTR initialProc;														/* system's first process */

	/* initialize the 4 new areas in low order memory with:
		stack pointer: last page of physical memory,
		PC: appropriate handler function in exceptions.c/interrupts.c (always set t9 to be the same),
		status reg: VM off, interrupts masked, kernel mode on, and interval timer enabled */
	initializeNewArea((state_PTR) SYSCALLNEWAREA, (memaddr) SyscallHandler, (memaddr) ramtop, statusRegValue);
	initializeNewArea((state_PTR) PROGRAMTRAPNEWAREA, (memaddr) ProgramTrapHandler, (memaddr) ramtop, statusRegValue);
	initializeNewArea((state_PTR) TLBMANAGEMENTNEWAREA, (memaddr) TLBManagementHandler, (memaddr) ramtop, statusRegValue);
	initializeNewArea((state_PTR) INTERRUPTNEWAREA, (memaddr) InterruptHandler, (memaddr) ramtop, statusRegValue); /* mikey had this one as a STST(), not sure if this the case, but if this doenst work maybe try this instead? */
	
	/* initialize phase 1 data structures */
	initPcbs();
	initASL();

	/* initialize phase 2 globals */
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	processStartTime = 0;
	readyQ = mkEmptyProcQ();

	/* initialize nucleus maintained semaphores */
	for(i=0; i < SEMCOUNT; i++){
		semaphoreArray[i] = 0;
	}

	initialProc = allocPcb();
	/* initialize first process's state with: 
		stack pointer: penultimate page of physical memory,
		PC: phase 2 test function (always set t9 to be the same)
		status reg: VM off, interrupts unmasked, kernel mode on, and timer enabled */
	initialProc->p_s.s_sp = ramtop - PAGESIZE;
	initialProc->p_s.s_t9 = (memaddr) test;	
	initialProc->p_s.s_pc = (memaddr) test;
	initialProc->p_s.s_status = ALLOFF | INITVMOFF | INTERRUPTSUNMASKED | KERNELON | TEBITON;

	processCount++;
	insertProcQ(&readyQ, initialProc);
	LDIT(INTERVALTIME); 											/* set initial time to interval timer */
	Scheduler();													/* schedule a process to run */
}

HIDDEN void initializeNewArea(state_PTR memArea, memaddr handlerName, memaddr sp, unsigned int status)
{
	/* initialize the new area */
	state_PTR newMemArea = memArea;
	newMemArea->s_pc = handlerName; 								/* set PC to handler function */
	newMemArea->s_t9 = handlerName; 								/* always set t9 to be the same as pc */
	newMemArea->s_sp = sp;											/* set stack pointer */
	newMemArea->s_status = status;									/* set status */
}
