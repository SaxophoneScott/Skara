/* Private/Local Methods */
HIDDEN void ReadFromTerminal(state_PTR syscallOld, char* destAddr);
HIDDEN void WriteToTerminal(state_PTR syscallOld, char* sourceAddr, int length);
/* HIDDEN void DiskPut(state_PTR syscallOld, int* blockAddr, int diskNum, int sector); */
/* HIDDEN void DiskGet(state_PTR syscallOld, int* blockAddr, int diskNum, int sector); */
HIDDEN void WriteToPrinter(state_PTR syscallOld, char* sourceAddr, int length);
HIDDEN void GetTOD(state_PTR syscallOld);
HIDDEN void UserTerminate(state_PTR syscallOld);

void UserSyscallHandler()
{
	/* who am I? */
	int asid = (getENTRYHI() && ASIDMASK) >> ASIDSHIFT;

	state_PTR syscallOld = userProcArray[asid-1].oldAreas[SYSCALLEXCEPTION];	/* state of the process issuing a syscall */
	syscallOld->s_pc += WORDLEN;									/* increment the pc, so the process will move on when it starts again */

	/* the syscall parameters/a registers */
	unsigned int l_a0 = syscallOld -> s_a0;
	unsigned int l_a1 = syscallOld -> s_a1;
	unsigned int l_a2 = syscallOld -> s_a2;
	unsigned int l_a3 = syscallOld -> s_a3;
	
	/* determine which syscall from a0 and handle it */
	switch(l_a0){ 
		/* SYS 9 */
		case READFROMTERMINAL:
			/* a1: virtual address to place data read */
			/* v0: if successful: number of characters transmitted, else negative of device's status */
			ReadFromTerminal(syscallOld, (char*) l_a1);
			break;
		/* SYS 10 */
		case WRITETOTERMINAL:
			/* a1: virtual address of first character */
			/* a2: length of the string */
			/* v0: if successful: number of characters transmitted, else negative of device's status */
			WriteToTerminal(syscallOld, (char*) l_a1, (int) l_a2);
			break;
		/* SYS 16 */
		case WRTIETOPRINTER:
			/* a1: virtual address of first character */
			/* a2: length of the string */
			WriteToPrinter(syscallOld, (char*) l_a1, (int) l_a2);
			break;
		/* SYS 17 */
		case GETTOD:
			GetTOD(syscallOld);
			break;
		/* SYS 18 */
		case USERTERMINATE:
			UserTerminate(syscallOld);
			break;
		/* unimplemented SYSCALL */
		default:
			/* what do we do here????? KILL? */
			/* we don't handle these, so pass up or die*/
			PassUpOrDie(syscallOld, SYSCALLEXCEPTION);

	}
}

void UserProgramTrapHandler()
{
	
}

HIDDEN void ReadFromTerminal(state_PTR syscallOld, char* destAddr){}

HIDDEN void WriteToTerminal(state_PTR syscallOld, char* sourceAddr, int length)
{
	char * s = sourceAddr;
	devregtr * base = (devregtr *) (TERM0ADDR);
	devregtr status;
	
	SYSCALL(PASSERN, (int)&term_mut, 0, 0);				/* P(term_mut) */
	while (*s != EOS) {
		*(base + 3) = PRINTCHR | (((devregtr) *s) << BYTELEN);
		status = SYSCALL(WAITIO, TERMINT, 0, 0);	
		if ((status & TERMSTATMASK) != RECVD)
			PANIC();
		s++;	
	}
	SYSCALL(VERHOGEN, (int)&term_mut, 0, 0);				/* V(term_mut) */
}

HIDDEN void DiskPut(state_PTR syscallOld, int* blockAddr, int diskNum, int sector){}

HIDDEN void DiskGet(state_PTR syscallOld, int* blockAddr, int diskNum, int sector){}

HIDDEN void WriteToPrinter(state_PTR syscallOld, char* sourceAddr, int length){}

HIDDEN void GetTOD(state_PTR syscallOld){}

HIDDEN void UserTerminate(state_PTR syscallOld){}