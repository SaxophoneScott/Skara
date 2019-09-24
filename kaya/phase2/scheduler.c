

void scheduler():
	pcb_PTR newProcess = removeProcQ(&readyQ);

	/* the readyQ is empty :( */
	if(currentP == NULL){

		/* there are no jobs in the system */
		if(processCount == 0){
			HALT();
		}
		/* there are jobs somewhere */
		else{
			if(softBlockCount == 0){
				PANIC();
			}
			else{
				/* suspended animation */
				WAIT();
				/* modify current state so wait bit is on and interrupts are enabled */
			}
		}

	}

	/* we got a ready process :) */
	else{
		currentProcess = newProcess;
		/* put value on clock = 1 quantum */
		setTIMER(5000); /* ?????? */
		/* store off TOD */
		currentProcess->p_startTime = STCK(TODLOADDR);
		LDST(&(currentProcess->p_s)); /* context switch! */
	}
