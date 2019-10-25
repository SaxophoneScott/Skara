/************************************************************* Scheduler.c *********************************************************************
* written by Scott Harrington and Kara Schatz                      																																								
* 																																																															
*																																																															
* 	Purpose: Implements a fully functional scheduler for the operating system using HALT(), PANIC(), WAIT(), and LoadState() as the four 
*	options for controlling the process flow and state of the operating system. It does so by taking the front process off the ready queue and 
*	allowing it to run. In the case that there is no such process, either HALT(), PANIC(), or WAIT() gets invoked. 	
*
*	The scheduler gets invoked in 3 instances:
*		1.) by main.c after all initial setup for the kaya operating system is complete
*		2.) by exceptions.c after the current process causes an exception and that exception raised causes the process to block 
*		3.) by interrupts.c after an interrupt occurs but there was no previous running process to return to																										
*																																																															
* 	The scheduler implements a simple round-robin scheduling algorithm giving each process a time slice of 5 milliseconds and guaranteeing that 
*	ready processes will all be given a turn to run, i.e. no starvation. 	
* 
* 	The scheduler is invoked by main() in initial.c and is the first external program called to get the operating system up and running.	
* 	The scheduler continues to schedule processes until the test() program finishes execution. After test() ends, the scheduler is halted, 
* 	as there are no processes in the system.																																							
* 																																																															
*	The scheduler includes exceptions.e, initial.e, pcb.e, types.h, const.h , and the umps2 library to  control execution of the operating system                               	
* 	The scheduler requires 4 phase 2 global variables: Ready Queue, Current Process, Process Count, and Soft-block Count																															                     																																																										*
*																																															                                                            	
***********************************************************************************************************************************************/



#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

void Scheduler()
/*
Scheduler takes the first process off the ready queue and performs an action depending on if it was successful in getting a ready process. 
If it was not successful, i.e. there are no ready processes to run, then it looks at the phase 2 global variables to determine which of the
following is appropriate: 
	done/halt: there are no processes in the system, i.e. processCount = 0;
	panic: there are processes in the system, but they are not blocked on semaphores, i.e. processCount > 0 and softBlockCount = 0
	wait: there are no ready processes, but there are some blocked on semaphores, i.e. processCount > 0 and softBlockCount > 0
When we halt, that means there are no process in the system, and the main God process, has finished. 
If it is successful, i.e. there is a ready process to run, then the current process is set to this new process, the timer is loaded with a 
quantum, the value on the time of day clock is stored off, and the new process's state is loaded, allowing it to run.
*/
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
