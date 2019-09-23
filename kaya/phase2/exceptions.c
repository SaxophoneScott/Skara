void syscallHandler(){
	state_t* syscallOld = (state_t*) SYSCALLOLDAREA;
	unsigned int l_a0 = syscallOld -> s_a0;
	unsigned int l_a1 = syscallOld -> s_a1;
	unsigned int l_a2 = syscallOld -> s_a2;
	unsigned int l_a3 = syscallOld -> s_a3;
	unsigned int l_v0;
	unsigned int l_v1;
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
			v0 = SYSCALL(CREATEPROCESS, state_t* statep)
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
void CreateProcess (state_t * statep)
/* have a baby */
/*  a1- address state
	allocate new pcb
	copy into p->p_s
	proccess count ++
	insert child(currentProc, p)
	instert procq(p1, &readyQue)
	set v0 
	pc= pc+1
	ldst($oldsys)
*/	
{
}
/* SYS2 */
void TerminateProcess( )
/* Kill the Process */
{

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
void LoadState()
{
		LDST(syscallOld);
}
/* instruction is greater than 8 */
void PassUporDie()
{

}

