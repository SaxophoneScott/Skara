void syscallHandler(){
	a0 = get a0 reg

	switch(a0){
		case CREATEPROCESS:
			/* a1: has physical address of processor state area */
			/* errors go in v0 as -1 or 0 otherwise */
			v0 = SYSCALL(CREATEPROCESS, state_t* statep)
		case TERMINATEPROCESS:

		case VERHOGEN:
		case PASSEREN:
		case EXCEPTIONSTATEVEC:
		case GETCPUTTIME: 
		case WAITFORCLOCK:
		case WAITFORIO:
	}
}

void programTrapHandler(){}

void TLBManagementHandler(){}