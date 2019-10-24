#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "/usr/local/include/umps2/umps/libumps.e"

HIDDEN void CreateProcess(state_PTR syscallOld, state_PTR newState);
HIDDEN void TerminateProcess(state_PTR syscallOld, pcb_PTR process);
HIDDEN void HoneyIKilledTheKids(state_PTR syscallOld, pcb_PTR p);
HIDDEN void Verhogen(state_PTR syscallOld, int* semaddr);
HIDDEN void Passeren(state_PTR syscallOld, int* semaddr);
HIDDEN void ExceptionStateVec(state_PTR syscallOld, unsigned int exceptionType, memaddr oldStateLoc, memaddr newStateLoc);
HIDDEN void GetCpuTime(state_PTR syscallOld);
HIDDEN void WaitForClock(state_PTR syscallOld);
HIDDEN void WaitForIo(state_PTR syscallOld, int lineNum, int deviceNum, int termRead);
HIDDEN void BlockHelperFunction(state_PTR syscallOld, int* semaddr, pcb_PTR process);
HIDDEN void PassUpOrDie(state_PTR oldState, int exceptionType);
HIDDEN void CopyState(state_PTR newState, state_PTR oldState);

void priv(unsigned int code)
{
	int y = 0;
	y++;
}

void SyscallHandler(){
	state_PTR syscallOld = (state_PTR) SYSCALLOLDAREA;	/* state of the process issuing a syscall */

	/* the syscall parameters/a registers */
	unsigned int l_a0 = syscallOld -> s_a0;
	unsigned int l_a1 = syscallOld -> s_a1;
	unsigned int l_a2 = syscallOld -> s_a2;
	unsigned int l_a3 = syscallOld -> s_a3;

	unsigned int KUMode = (syscallOld->s_status) & KERNELOFF;	/* will be all 0s if the process was in kernel mode */
	int userMode;

	if(KUMode == 0)
	{
		userMode = FALSE;
	}
	else
	{
		userMode = TRUE;
	}

	syscallOld->s_pc += WORDLEN;								/* increment the pc, so the process will move on when it starts again */

	/* case: the process had permission to make the syscall (1-8 with kernel mode or >9) */
	/* if(l_a0 > 8 || (!userMode && 1 <= l_a0 && l_a0 <= 8)) */
	if(userMode && 1 <= l_a0 && l_a0 <= 8)
	{
		priv(PRIVILEDGEDINSTR);
		/* treat this as a program trap */
		state_PTR programTrapOld = (state_PTR) PROGRAMTRAPOLDAREA;
		CopyState(programTrapOld, syscallOld);
		/*if((currentProcess->oldAreas[PROGRAMTRAPEXCEPTION] == NULL) || (currentProcess->newAreas[PROGRAMTRAPEXCEPTION] == NULL)){
			programTrapOld = (state_PTR) PROGRAMTRAPOLDAREA;
		} else {
			programTrapOld = currentProcess->oldAreas[PROGRAMTRAPEXCEPTION];
		}*/
		programTrapOld->s_cause = ALLOFF | PRIVILEDGEDINSTR;		/* set cause as priviledged instruction */
		ProgramTrapHandler();
	}
	/* case: priviledged call but no permissions (i.e. 1-8 in user mode) */
	else
	{
		/* determine which syscall from a0 and handle it */
		switch(l_a0){ 
			/* SYS 1 */
			case CREATEPROCESS:
				/* a1: physical address of processor state */
				/* v0: 0 if successful, -1 otherwise */
				/* non blocking */
				CreateProcess(syscallOld, (state_PTR) l_a1);
				break;
			/* SYS 2 */
			case TERMINATEPROCESS:
				/* blocking */
				TerminateProcess(syscallOld, currentProcess);
				break;
			/* SYS 3 */
			case VERHOGEN:
				/* a1: physcial address of semaphore */
				/* non blocking*/
				Verhogen(syscallOld, (int*) l_a1);
				break;
			/* SYS 4 */
			case PASSEREN:
				/* a1: physical address of semaphore */
				/* sometimes blocking*/
				Passeren(syscallOld, (int*) l_a1);
				break;
			/* SYS 5 */
			case EXCEPTIONSTATEVEC:
				/* a1: the type of exception */
				/* a2: the address to use as old area */
				/* a3: the address to use as new area */
				/* non blocking*/
			 	ExceptionStateVec(syscallOld, l_a1, (memaddr) l_a2, (memaddr) l_a3);
			 	break;
			/* SYS 6 */
			case GETCPUTIME: 
				/* v0: the processor time used */
				/* non blocking*/
				GetCpuTime(syscallOld);
				break;
			/* SYS 7 */
			case WAITFORCLOCK:
				/* blocking*/
				WaitForClock(syscallOld);
				break;
			/* SYS 8 */
			case WAITFORIO:
				/* a1: interrupt line num */
				/* a2: device line num */
				/* a3: 1 for terminal read, 0 otherwise */
				/* blocking */
				WaitForIo(syscallOld, (int) l_a1, (int) l_a2, (int) l_a3);
				break;
			/* SYS 9 and above */
			default:
				/* we don't handle these, so pass up or die*/
				PassUpOrDie(syscallOld, SYSCALLEXCEPTION);

		}
	}
}

void ProgramTrapHandler()
{
	state_PTR programTrapOld = (state_PTR) PROGRAMTRAPOLDAREA;
	PassUpOrDie(programTrapOld, PROGRAMTRAPEXCEPTION);
}

void TLBManagementHandler()
{
	state_PTR TLBManagementOld = (state_PTR) TLBMANAGEMENTOLDAREA;
	PassUpOrDie(TLBManagementOld, TLBEXCEPTION);
}

/* helper function to localize potential LDST's */
void LoadState(state_PTR processState)
{
	LDST(processState);
}

void IncrementProcessTime(pcb_PTR process)
{
	cpu_t endTime;											/* current time  */
	STCK(endTime); 
	process->p_totalTime += (endTime - processStartTime); 	/* increment processor time used based on process start time */
}

/* SYS1 */
HIDDEN void CreateProcess(state_PTR syscallOld, state_PTR newState)
{
	pcb_PTR newProcess = allocPcb();				/* get a new process */

	/* case: there were no available processes to allocate */
	if (newProcess == NULL){
		syscallOld->s_v0 = -1;						/* indicate failed creation attempt */
	/* case: we got a new process :) */
	} else {
		CopyState(&(newProcess->p_s), newState);	/* load the state of the new process */
		insertChild(currentProcess, newProcess);
		insertProcQ(&readyQ, newProcess);
		processCount++;
		syscallOld->s_v0 = 0;						/* indicate successful creation attempt */
	}

	LoadState(syscallOld);							/* let the calling process start running again */
}

/* SYS2 */
HIDDEN void TerminateProcess(state_PTR syscallOld, pcb_PTR process)
{
	HoneyIKilledTheKids(syscallOld, process);		/* recursively kill all prodigy */
	/*outProcQ(&readyQ, process);
	processCount--;
	IncrementProcessTime(process);
	freePcb(process);*/
	Scheduler();									/* schedule a new process to run */
}

/* helper function for TerminateProcess()
	uses recurision to kill all pf the children and their children  and thier children etc 
	of a pcb p*/
HIDDEN void HoneyIKilledTheKids(state_PTR syscallOld, pcb_PTR p)
{
	/* there's still more kids to kill */
	while(!(emptyChild(p)))
	{
		/* pcb_PTR kid = removeChild(p); */
		HoneyIKilledTheKids(syscallOld, removeChild(p));
	}

	/* case: kill the current process */
	if(p == currentProcess)
	{
		outChild(p);										/* make it no longer it's parent's kid */
	}
	/* case: kill a ready process */
	else if(p->p_semAdd == NULL)
	{
		outProcQ(&readyQ, p);								/* take it off the readyQ */
	}
	/* case: kill a blocked process */
	else
	{
		int* semaddr = p->p_semAdd;							/* the process's semaddr */
		int* firstDevice = &(semaphoreArray[0]);			/* first device's semaddr */
		int* lastDevice = &(semaphoreArray[SEMCOUNT - 2]); 	/* last device's semaddr */
		outBlocked(p);										/* unblock it */

		/* case: blocked on device sema4 */
		if(firstDevice <= semaddr && semaddr <= lastDevice)
		{
			softBlockCount--;
		}
		/* case: blocked on non-device sema4 */
		else
		{
			*semaddr = *semaddr + 1;
		}
	}

	processCount--;
	freePcb(p);
}

/*SYS3 */
/* increments the sema4 */
HIDDEN void Verhogen(state_PTR syscallOld, int* semaddr)
{
	*(semaddr) = *semaddr + 1;

	/* case: signal that a process can unblock now */
	if((*semaddr) <= 0)
	{
		pcb_PTR p = removeBlocked(semaddr);

		/* case: we found a process to unblock */
		if(p != NULL)
		{
			insertProcQ(&readyQ, p);			/* now it's ready */
		}
	}

	LoadState(syscallOld);						/* let the calling process start running again */
}

/*SYS4*/
/* decrements the sema4 */
HIDDEN void Passeren(state_PTR syscallOld, int* semaddr)
{
	(*semaddr) = *semaddr - 1;	

	/* case: wait/block until signaled later */
	if((*semaddr) <0)
	{
		BlockHelperFunction(syscallOld, semaddr, currentProcess);
	}
	/* case: no need to wait, keep on chugging */
	else
	{
		LoadState(syscallOld);					/* let the calling process start running again */
	}
}

/*SYS5*/
HIDDEN void ExceptionStateVec(state_PTR syscallOld, unsigned int exceptionType, memaddr oldStateLoc, memaddr newStateLoc)
{
	/* case: process has not specified an exception state vector yet, so let it do so now */
	if(currentProcess->oldAreas[exceptionType]==NULL && currentProcess->newAreas[exceptionType]==NULL)
	{
		currentProcess->oldAreas[exceptionType] = (state_PTR) oldStateLoc;
		currentProcess->newAreas[exceptionType] = (state_PTR) newStateLoc;
		LoadState(syscallOld);
	}
	/* case: process has already specified one, so kill it */
	else
	{
		TerminateProcess(syscallOld, currentProcess);
	}
}

/*SYS6*/
HIDDEN void GetCpuTime(state_PTR syscallOld)
{
	cpu_t time = currentProcess->p_totalTime;		/* time used before this turn */
	cpu_t endTime;									/* current time */
	STCK(endTime);
	time += (endTime - processStartTime);			/* add time used before and during this turn */ 
	syscallOld->s_v0 = time;						/* return total time used */
	LoadState(syscallOld);							/* let the calling process start running again */
}

/*SYS7*/
HIDDEN void WaitForClock(state_PTR syscallOld)
{
	int* semaphore = &(semaphoreArray[SEMCOUNT - 1]); 	/* pseudo-clock timer's semaddr */
	Passeren(syscallOld, semaphore);					/* P the sema4 and block the process */
}

/*SYS8*/
HIDDEN void WaitForIo(state_PTR syscallOld, int lineNum, int deviceNum, int termRead)
{
	int index = (lineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + deviceNum;		/* index of device */
	int* semaddr;																	/* device's semaddr */
	device_t* deviceAddr;															/* device's register address */
	
	/* case: it's a terminal read */
	if(termRead)	
	{
		index += NUMDEVICESPERTYPE;							/* modify index to get second set of terminal device sema4s (i.e. receive sema4s) */
	}

	semaddr = &semaphoreArray[index];
	/* Passeren(syscallOld, &semaphoreArray[index]); */
	(*semaddr)--;		

	/* case: block the process until it gets IO */								
	if(*(semaddr) <0)
	{
		BlockHelperFunction(syscallOld, semaddr, currentProcess);
	}
	/* case: ERROR ERROR */
	else
	{
		/* terminate??? */
		TerminateProcess(syscallOld, currentProcess);
	}

	deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (deviceNum * DEVICESIZE));
	syscallOld->s_v0 = deviceAddr->d_status;			/* return device's status */
}	

/* helper function to localize blocking */
HIDDEN void BlockHelperFunction(state_PTR syscallOld, int* semaddr, pcb_PTR process)
{
	IncrementProcessTime(process);					/* update accumulated cpu time used */
	CopyState(&(process->p_s), syscallOld);			/* update the process's state in case of any changes */
	insertBlocked(semaddr, process);
	softBlockCount++;
	currentProcess = NULL;
	Scheduler();									/* schedule a new process to run */
}

HIDDEN void PassUpOrDie(state_PTR oldState, int exceptionType)
{
	/* has a sys5 for that trap type been called?
		if that area == null, then no:
			terminate the process and all its offspring
		yes:
			copy state that caused exception (oldxxx - system level) to the location specified in the PCB
			LDST(current->newxxx) */
	/* case: DIE - an exception state vector (SYS5) has not be set up */
	if((currentProcess->oldAreas[exceptionType] == NULL) || (currentProcess->newAreas[exceptionType] == NULL)){
		TerminateProcess(oldState, currentProcess);
	/* case: PASS UP -  an exception state vector has been set up */
	} else {
		CopyState(currentProcess->oldAreas[exceptionType], oldState);
		CopyState(&(currentProcess->p_s), currentProcess->newAreas[exceptionType]);
		LoadState(currentProcess->newAreas[exceptionType]);							/* pass control up to the process's exception handler */
	}
}

HIDDEN void CopyState(state_PTR newState, state_PTR oldState)
{
	int i;														/* loop control variable to copying the registers */
	/* copy all scalar fields */
	newState->s_asid = oldState->s_asid;
	newState->s_cause = oldState->s_cause;
	newState->s_status = oldState->s_status;
	newState->s_pc = oldState->s_pc;
	/* copy all registers */
	for(i = 0; i < STATEREGNUM; i++)
	{
		newState->s_reg[i] = oldState->s_reg[i];
	}
}
