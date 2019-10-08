

void scheduler():
	pcb_PTR newProcess = removeProcQ(&readyQ);

	/* the readyQ is empty :( */
	if(newProcess == NULL){
		/* there are no jobs in the system */
		if(processCount == 0){
			currentProcess = NULL;
			HALT();
		}
		/* there are jobs somewhere */
		else{
			if(softBlockCount == 0){
				currentProcess = NULL;
				PANIC();
			}
			else{
				/* suspended animation */
				/* currentProcess->p_s->s_status = ALLOFF | CURRINTERRUPTSUNMASKED | INTERRUPTMASKON; */
				setStatus(ALLOFF | CURRINTERRUPTSUNMASKED | INTERRUPTMASKON);
				currentProcess = NULL;
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
		/* currentProcess->p_startTime = STCK(TODLOADDR); */
		STCK(processStartTime);
		LoadState(&(currentProcess->p_s)); /* context switch! */
	}