/************************************************************* Scheduler.c *********************************************************************
* written by Scott Harrington and Kara Schatz                      																																								*
* 																																																															*
*																																																															*
* 	Purpose: Implements A fully functional scheduler, using Halt(), Wait(), Panic(), and LoadState() 																									*
*  to control the process flow of the Operating system. The scheduler controls the flow of processes 																								*
*	after the current process causes an exception and the exception raised causes the process to block, operating 																			* 
*	when the interrupt handler causes an exception. The schedular controls the control flow of the 																									*
*	operating system, as it takes the top process off the ready queue, and allows it to run. 																												*
*																																																															*
* 	The Scheduler implements a simple round-robin, with each process having a time slice of 5 milliseconds that guaranteess that ready process will excute when 	*
*	when it is their turn. 																																																						*
* 																																																															*
*	The scheduler includes exceptions.e, initial.e, pcb.e, types.h, const.h , and the umps2 library to  control execution of the operating system                               	*
* 	The scheduler implements 4 global variables, ReadyQueue, Current Process, Process Count, and Soft-block count 																		*
*   																																																														*
*    The scheduler is the first external program that Intial.c calls.     																																				*			
*																																																															*
*	The Scheduler runs test() as its main program, and continues to schedule processes until the test() program finishes execution. 													*
* After test() ends, the scheduler is halted, as there are no process in the system.																															*                      																																																										*
*																																															                                                            	*
***********************************************************************************************************************************************/



#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* Void Scheduler()

	Scheduler takes the first process off the ready queue and performs an action depending on if it was successful on taking a process off. 
	If it was not successful, then look at the global variables set in initial.c to determine if we are 
	done/halt - no currentprocess  = 0;
	panic - currentprocess >1 and softblock count = 0; 
	wait - currentprocess >1 and softblock count >1

	when we halt, that means there are no process in the scheduler, and means the main God process, has finished. 

	when it is succesful in getting a process, then set the current process to this new process, set the timer, store the time of day clock,
	and load the processer state, and allow it to run.
*/
void Scheduler()
{
	pcb_PTR newProcess = removeProcQ(&readyQ);						/* try to get a new process */

	/* case: there are no ready processes :( */
	if(newProcess == NULL){
		/* case: there are no jobs in the system */
		if(processCount == 0){
			currentProcess = NULL;
			HALT();
		}
		/* case: there are jobs somewhere */
		else{
			/* case: we don't know where the jobs are though */
			if(softBlockCount == 0){
				currentProcess = NULL;
				PANIC();
			}
			/* case: they're just blocked, so we'll wait for them to unblock */
			else{
				/* suspended animation */
				/* enable all interrupts so that processes will get unblocked */
				setSTATUS(ALLOFF | CURRINTERRUPTSUNMASKED | INTERRUPTMASKON);		
				currentProcess = NULL;
				setTIMER(SUSPENDTIME);
				WAIT();
			}
		}

	}

	/* case: we got a ready process :) */
	else{
		currentProcess = newProcess;
		setTIMER(PLTTIME);											/* put value on clock = 1 quantum for process's turn */
		STCK(processStartTime);										/* store off current time, so we can track time used */
		LoadState(&(currentProcess->p_s)); 							/* start the new process -> context switch! */
	}
}
