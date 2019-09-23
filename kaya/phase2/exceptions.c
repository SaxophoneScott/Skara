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
		case VERHOGEN:
		/* non blocking*/
		case PASSEREN:
		/* sometimes blocking*/
		case EXCEPTIONSTATEVEC:
		/* non blocking*/
		case GETCPUTTIME: 
		/* non blocking*/
		case WAITFORCLOCK:
		/* blocking*/
		case WAITFORIO:
		/* blocking */
		default:
		/* pass up or die*/
			PassUporDie();
	}
}

void programTrapHandler(){}

void TLBManagementHandler(){}


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
void TerminateProcess( )
/* Kill the Process */
/* remove from the proqQ
 while (!emptyChild(currentProcess)) -> removeChild(currentProccess) 
 proccessCount -- for each child removed
freePcb for each one */
{
	HoneyIKilledTheKids(currentProcess);
	outProcQ(&readyQ, currentProcess);
	processCount--;
	freePcb(currentProcess);
	Scheduler();

}

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
void Verhogen()
{

}
/*SYS4*/
void Passeren()
{

}
/*SYS5*/
void ExceptionStateVec()
{

}
/*SYS6*/
void GetCpuTime ()
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
void BlockHelperFunction()
/* insertBlock(SemADD, current)
	softBlockedCount++;
	Schedular() */
{

}

/* instruction is greater than 8 */
void PassUpOrDie()
{

}

