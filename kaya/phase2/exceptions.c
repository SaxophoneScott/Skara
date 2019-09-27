void syscallHandler(){
	pc=pc+4;
	state_t* syscallOld = (state_t*) SYSCALLOLDAREA;
	unsigned int l_a0 = syscallOld -> s_a0;
	/*unsigned int l_a1 = syscallOld -> s_a1;
	unsigned int l_a2 = syscallOld -> s_a2;
	unsigned int l_a3 = syscallOld -> s_a3;
	unsigned int l_v0;
	unsigned int l_v1;*/
	/* if l_a0 <= 8 and l_a0 >= 1 and kup =!0 
	prgram trap
	copy state from oldSys into  OldProgram
	change old program cause to priviledged instruction issue (RI) (10)
	Call myProgramTrap()

	else :

	*/ 
	switch(a0){
		case CREATEPROCESS:
			/* a1: has physical address of processor state area */
			/* errors go in v0 as -1 or 0 otherwise */
			/* non blocking*/
			CreateProcess(syscallOld);
		case TERMINATEPROCESS:
		/* blocking */
			TerminateProcess(currentProcess);
		case VERHOGEN:
		/* non blocking*/
			Verhogen(syscallOld);
		case PASSEREN:
		/* sometimes blocking*/
			Passeren(syscallOld);
		case EXCEPTIONSTATEVEC:
		/* non blocking*/
		/* pass up or die? */
		 	ExceptionStateVec(syscallOld);
		case GETCPUTTIME: 
		/* non blocking*/
		case WAITFORCLOCK:
		/* blocking*/
		case WAITFORIO:
		/* blocking */
		default:
		/* pass up or die*/
			PassUporDie(syscallOld, SYCALLEXCEPTION);
	}
}

void programTrapHandler()
{
	PassUpOrDie(syscallOld, PROGRAMTRAPEXCEPTION);
}

void TLBManagementHandler()
{
	PassUpOrDie(syscallOld, TLBEXCEPTION);
}


/* SYS1 */
void CreateProcess (state_t* syscallOld)
/* have a baby */
/*  a1- address state
	allocate new pcb
	copy into p->p_s
	proccess count ++
	insert child(currentProc, p)
	instert procq(p1, &readQ)
	set v0 
	ldst($oldsys)
*/	
{
	unsigned int newState = syscallOld->s_a1;
	newProcess = allocPcb();
	if (newProcess == NULL){
		syscallOld->s_v0 = -1;
	} else {
		newProcess->p_s = *newState;
		insertChild(currentProcess, newProcess);
		insertProcQ(&readyQ, newProcess);
		processCount++;
		syscallOld->s_v0 = 0;
	}
	LoadState(syscallOld);
}
/* SYS2 */
void TerminateProcess(pcb_PTR process)
/* Kill the Process */
/* remove from the proqQ
 while (!emptyChild(currentProcess)) -> removeChild(currentProccess) 
 proccessCount -- for each child removed
freePcb for each one */
{
	HoneyIKilledTheKids(process);
	outProcQ(&readyQ, process);
	processCount--;
	IncrementProcessTime(process);
	freePcb(process);
	Scheduler();

}
/* helper function for TerminateProcess() 
	uses recurision to kill all pf the children and their children  and thier children etc 
	of a pcb p*/
void HoneyIKilledTheKids(pcb_PTR p)
{
	while(!(emptyChild(p)))
	{
		pcb_PTR kid = removeChild(p);
		HoneyIKilledTheKids(kid);
		outProcQ( &readyQ , kid);
		processCount--;
		freePcb(kid);
	}
}
/*SYS3 */
void Verhogen(state_t * syscallOld)
{
	/* increments the semaphore */
	memaddr semaddr = syscallOld->s_a1;
	(*semaddr)++; 
	if(*(semaddr) <= 0)
	{
		pcb_PTR p=removeBlocked(semaddr);
		insertProcQ(&readyQ, p);	
	}
	LoadState(syscallOld);
}
/*SYS4*/
void Passeren(state_t* syscallOld)
{
	/* decrements the semaphore */
	memaddr semaddr = syscallOld->s_a1;
	(*semaddr)--; 
	if(*(semaddr) <0)
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
void ExceptionStateVec(state_t * syscallOld)
{
	unsigned int exceptionType = syscallOld->s_a1;
	memaddr oldState = syscallOld->s_a2;
	memaddr newState = syscallOld->s_a3;

	if(currentProcess->oldAreas[exceptionType]==NULL && currentProcess->newAreas[exceptionType]==NULL)
	{
		/* hasn't been requested yets :) */
		currentProcess->oldAreas[exceptionType]=oldState;
		currentProccess->newAreas[exceptionType]=newState;
	}
	else
	{
		/* it was already requested :( */
		TerminateProcess(currentProcess);
	}

}
/*SYS6*/
void GetCpuTime()
{

}
/*SYS7*/
void WaitForClock()
{

}
/*SYS8*/
void WaitForIo()
{

}
/* helper function to localize potential LDST's */
void LoadState(memaddr processState)
{
		LDST(processState);
}
/* helper function to localize blocking */
void BlockHelperFunction(state_t * syscallOld, memaddr semaddr, pcb_PTR process)
/* insertBlock(SemADD, current)
	softBlockedCount++;
	Scheduler() */
{
	IncrementProcessTime(process);
	insertBlocked(semaddr, process);
	softBlockedCount++;
	Scheduler();
}

/* instruction is greater than 8 */
/* sys5 */
void PassUpOrDie(state_t * syscallOld, int exceptionType)
{
	/* has a sys5 for that trap type been called?
		if that area == null, then no:
			terminate the process and all its offspring
		yes:
			copy state that caused exception (oldxxx - system level) to the location specified in the PCB
			LDST(current->newxxx) */
	if((currentProcess->oldAreas[exceptionType] == NULL) || (currentProcess->newAreas[exceptionType] == NULL)){
		/* TERMINATE BC SYS 5 WASNT CALLED */
		TerminateProcess(currentProcess);
	} else {
			*(currentProcess->oldAreas[exceptionType]) = *(syscallOld);
			LoadState(currentProcess->newAreas[exceptionType]);
		}
	}

void IncrementProcessTime(pcb_PTR process)
{
	cpu_t endTime;
	STCK(endTime);
	process->p_totalTime += (endTime - processStartTime);
}