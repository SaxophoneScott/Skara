#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

void Scheduler()
{
	pcb_PTR newProcess = removeProcQ(&readyQ);	/* try to get a new process */

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
		setTIMER(PLTTIME);					/* put value on clock = 1 quantum for process's turn */
		STCK(processStartTime);				/* store off current time, so we can track time used */
		LoadState(&(currentProcess->p_s)); 	/* start the new process -> context switch! */
	}
}
