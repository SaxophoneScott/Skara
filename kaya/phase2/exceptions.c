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

void SyscallHandler(){
	state_PTR syscallOld = (state_PTR) SYSCALLOLDAREA;
	state_PTR programTrapOld = (state_PTR) PROGRAMTRAPOLDAREA;
	unsigned int l_a0 = syscallOld -> s_a0;
	unsigned int l_a1 = syscallOld -> s_a1;
	unsigned int l_a2 = syscallOld -> s_a2;
	unsigned int l_a3 = syscallOld -> s_a3;
	/*unsigned int l_v0;
	unsigned int l_v1;*/
	int userMode = currentProcess->p_s.s_status & KERNELOFF;
	syscallOld->s_pc += 4;
	if(l_a0 > 8 || (!userMode && 1 <= l_a0 && l_a0 <= 8))
	{
		switch(l_a0){
			case CREATEPROCESS:
				/* a1: has physical address of processor state area */
				/* errors go in v0 as -1 or 0 otherwise */
				/* non blocking*/
				CreateProcess(syscallOld, (state_PTR) l_a1);
			case TERMINATEPROCESS:
			/* blocking */
				TerminateProcess(syscallOld, currentProcess);
			case VERHOGEN:
			/* non blocking*/
				Verhogen(syscallOld, (int*) l_a1);
			case PASSEREN:
			/* sometimes blocking*/
				Passeren(syscallOld, (int*) l_a1);
			case EXCEPTIONSTATEVEC:
			/* non blocking*/
			/* pass up or die? */
			 	ExceptionStateVec(syscallOld, l_a1, (memaddr) l_a2, (memaddr) l_a3);
			case GETCPUTIME: 
			/* non blocking*/
				GetCpuTime(syscallOld);
			case WAITFORCLOCK:
			/* blocking*/
				WaitForClock(syscallOld);
			case WAITFORIO:
			/* blocking */
				WaitForIo(syscallOld, (int) l_a1, (int) l_a2, (int) l_a3);
			default:
			/* pass up or die*/
				PassUpOrDie(syscallOld, SYSCALLEXCEPTION);
		}
	}
	else
	/* in user mode but made priveledged call */
	{
		/* if l_a0 <= 8 and l_a0 >= 1 and kup =!0 
		prgram trap
		copy state from oldSys into  OldProgram
		change old program cause to priviledged instruction issue (RI) (10)
		Call myProgramTrap() */
		CopyState(programTrapOld, syscallOld);
		programTrapOld->s_cause = ALLOFF | PRIVILEDGEDINSTR;
		ProgramTrapHandler();
	}
}

void ProgramTrapHandler()
{
	state_PTR programTrapOld = (state_PTR) PROGRAMTRAPOLDAREA;
	programTrapOld->s_pc += 4; /* ??? */
	PassUpOrDie(programTrapOld, PROGRAMTRAPEXCEPTION);
}

void TLBManagementHandler()
{
	state_PTR TLBManagementOld = (state_PTR) TLBMANAGEMENTOLDAREA;
	TLBManagementOld->s_pc += 4; /* ??? */
	PassUpOrDie(TLBManagementOld, TLBEXCEPTION);
}


/* SYS1 */
HIDDEN void CreateProcess (state_PTR syscallOld, state_PTR newState)
/* have a baby */
/*  a1- address state
	allocate new pcb
	copy into p->p_s
	proccess count ++
	insert child(currentProc, p)
	instert procq(p1, &readyQ)
	set v0
	ldst($oldsys)
*/
{
	/* unsigned int newState = syscallOld->s_a1; */
	pcb_PTR newProcess = allocPcb();
	if (newProcess == NULL){
		syscallOld->s_v0 = -1;
	} else {
		CopyState(&(newProcess->p_s), newState);
		insertChild(currentProcess, newProcess);
		insertProcQ(&readyQ, newProcess);
		processCount++;
		syscallOld->s_v0 = 0;
	}
	LoadState(syscallOld);
}
/* SYS2 */
HIDDEN void TerminateProcess(state_PTR syscallOld, pcb_PTR process)
/* Kill the Process */
/* remove from the proqQ
 while (!emptyChild(currentProcess)) -> removeChild(currentProccess) 
 processCount -- for each child removed
freePcb for each one */
{
	HoneyIKilledTheKids(syscallOld, process);
	/*outProcQ(&readyQ, process);
	processCount--;
	IncrementProcessTime(process);
	freePcb(process);*/
	Scheduler();

}
/* helper function for TerminateProcess()
	uses recurision to kill all pf the children and their children  and thier children etc 
	of a pcb p*/
HIDDEN void HoneyIKilledTheKids(state_PTR syscallOld, pcb_PTR p)
{
	while(!(emptyChild(p)))
	{
		/* pcb_PTR kid = removeChild(p); */
		HoneyIKilledTheKids(syscallOld, removeChild(p));
	}
	if(p == currentProcess)
	{
		outChild(p);
	}
	if(*(p->p_semAdd) == 0)
	/* it's on the readyQ */
	{
		outProcQ(&readyQ, p);
	}
	else
	/* it's blocked */
	{
		int* semaddr = p->p_semAdd;
		int firstDevice = semaphoreArray[0];
		int lastDevice = semaphoreArray[SEMCOUNT - 2]; /* last device is second to last elem bc timer is last elem */
		outBlocked(p);
		if(&firstDevice <= semaddr && semaddr <= &lastDevice)
		/* blocked on a device sema4 */
		{
			softBlockCount--;
		}
		else
		/* blocked on a non-device sema4 */ 
		{
			Verhogen(syscallOld, semaddr);
		}
	}
	processCount--;
	freePcb(p);
}
/*SYS3 */
HIDDEN void Verhogen(state_PTR syscallOld, int* semaddr)
{
	/* increments the semaphore */
	/* memaddr semaddr = syscallOld->s_a1; */
	*(semaddr) = *semaddr + 1;
	if((*semaddr) <= 0)
	{
		pcb_PTR p=removeBlocked(semaddr);
		insertProcQ(&readyQ, p);
	}
	LoadState(syscallOld);
}
/*SYS4*/
HIDDEN void Passeren(state_PTR syscallOld, int* semaddr)
{
	/* decrements the semaphore */
	/* memaddr semaddr = syscallOld->s_a1; */
	(*semaddr) = *semaddr - 1;
	if((*semaddr) <0)
	{
		/* block the proccess */
		BlockHelperFunction(syscallOld, semaddr, currentProcess);
	}
	else
	{
		/* otherwise continue operation */
		LoadState(syscallOld);
	}
}
/*SYS5*/
HIDDEN void ExceptionStateVec(state_PTR syscallOld, unsigned int exceptionType, memaddr oldStateLoc, memaddr newStateLoc)
{
	/* unsigned int exceptionType = syscallOld->s_a1;
	memaddr oldState = syscallOld->s_a2;
	memaddr newState = syscallOld->s_a3; */

	if(currentProcess->oldAreas[exceptionType]==NULL && currentProcess->newAreas[exceptionType]==NULL)
	{
		/* hasn't been requested yet :) */
		currentProcess->oldAreas[exceptionType] = (state_PTR) oldStateLoc;
		currentProcess->newAreas[exceptionType] = (state_PTR) newStateLoc;
	}
	else
	{
		/* it was already requested :( */
		TerminateProcess(syscallOld, currentProcess);
	}

}
/*SYS6*/
HIDDEN void GetCpuTime(state_PTR syscallOld)
{
	cpu_t time = currentProcess->p_totalTime;
	cpu_t endTime;
	STCK(endTime);
	time += (endTime - processStartTime);
	/* process->p_totalTime += (endTime - processStartTime); */
	syscallOld->s_v0 = time;
}
/*SYS7*/
HIDDEN void WaitForClock(state_PTR syscallOld)
{
	int semaphore = semaphoreArray[SEMCOUNT - 1]; /*pseudo-clock timer is last semaphore */
	Passeren(syscallOld, &semaphore);
}
/*SYS8*/
HIDDEN void WaitForIo(state_PTR syscallOld, int lineNum, int deviceNum, int termRead)
{
	int index = (lineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + deviceNum;
	int* semaddr;
	device_t* deviceAddr;
	if(termRead)
	{
		index += NUMDEVICESPERTYPE;
	}
	semaddr = &semaphoreArray[index];
	/* Passeren(syscallOld, &semaphoreArray[index]); */
	(*semaddr)--;
	if(*(semaddr) <0)
	{
		/* block the proccess */
		BlockHelperFunction(syscallOld, semaddr, currentProcess);
	}
	else
	{
		/* ERROR. ERROR? */
		/* terminate??? */
		TerminateProcess(syscallOld, currentProcess);
	}
	deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (deviceNum * DEVICESIZE));
	syscallOld->s_v0 = deviceAddr->d_status;
}
/* helper function to localize potential LDST's */
void LoadState(state_PTR processState)
{
	LDST(processState);
}
/* helper function to localize blocking */
HIDDEN void BlockHelperFunction(state_PTR syscallOld, int* semaddr, pcb_PTR process)
/* insertBlock(SemADD, current)
	softBlockedCount++;
	Scheduler() */
{
	IncrementProcessTime(process);
	insertBlocked(semaddr, process);
	softBlockCount++;
	currentProcess = NULL;
	Scheduler();
}

/* instruction is greater than 8 */
/* sys5 */
HIDDEN void PassUpOrDie(state_PTR oldState, int exceptionType)
{
	/* has a sys5 for that trap type been called?
		if that area == null, then no:
			terminate the process and all its offspring
		yes:
			copy state that caused exception (oldxxx - system level) to the location specified in the PCB
			LDST(current->newxxx) */
	if((currentProcess->oldAreas[exceptionType] == NULL) || (currentProcess->newAreas[exceptionType] == NULL)){
		/* TERMINATE BC SYS 5 WASNT CALLED */
		TerminateProcess(oldState, currentProcess);
	} else {
		/* *(currentProcess->oldAreas[exceptionType]) = *(oldState); */
		CopyState(currentProcess->oldAreas[exceptionType], oldState);
		LoadState(currentProcess->newAreas[exceptionType]);
	}
}

void IncrementProcessTime(pcb_PTR process)
{
	cpu_t endTime;
	STCK(endTime);
	process->p_totalTime += (endTime - processStartTime);
}

HIDDEN void CopyState(state_PTR newState, state_PTR oldState)
{
	int i;
	newState->s_asid = oldState->s_asid;
	newState->s_cause = oldState->s_cause;
	newState->s_status = oldState->s_status;
	newState->s_pc = oldState->s_pc;
	for(i = 0; i < STATEREGNUM; i++)
	{
		newState->s_reg[i] = oldState->s_reg[i];
	}
}
