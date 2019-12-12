
#include "../h/const.h"
#include "../h/types.h"
#include "../e/initProc.e"
#include "../e/sysSupport.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* Private/Local Methods */
HIDDEN void ReadFromTerminal(int asid, state_PTR syscallOld, char* destAddr);
HIDDEN void WriteToTerminal(int asid, state_PTR syscallOld, char* sourceAddr, int length);
/* HIDDEN void DiskPut(state_PTR syscallOld, int* blockAddr, int diskNum, int sector); */
/* HIDDEN void DiskGet(state_PTR syscallOld, int* blockAddr, int diskNum, int sector); */
HIDDEN void WriteToPrinter(int asid, state_PTR syscallOld, char* sourceAddr, int length);
HIDDEN void GetTOD(state_PTR syscallOld);
HIDDEN void UserTerminate(int asid, state_PTR syscallOld);

void UserSyscallHandler()
{
	/* int asid;
	state_PTR syscallOld; */

	/* who am I? */
	int asid = (getENTRYHI() && ASIDMASK) >> ASIDSHIFT;

	state_PTR syscallOld = &(userProcArray[asid-1].oldAreas[SYSCALLEXCEPTION]);	/* state of the process issuing a syscall */
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
			ReadFromTerminal(asid, syscallOld, (char*) l_a1);
			break;
		/* SYS 10 */
		case WRITETOTERMINAL:
			/* a1: virtual address of first character */
			/* a2: length of the string */
			/* v0: if successful: number of characters transmitted, else negative of device's status */
			WriteToTerminal(asid, syscallOld, (char*) l_a1, (int) l_a2);
			break;
		/* SYS 16 */
		case WRITETOPRINTER:
			/* a1: virtual address of first character */
			/* a2: length of the string */
			WriteToPrinter(asid, syscallOld, (char*) l_a1, (int) l_a2);
			break;
		/* SYS 17 */
		case GETTOD:
			GetTOD(syscallOld);
			break;
		/* SYS 18 */
		case USERTERMINATE:
			UserTerminate(asid, syscallOld);
			break;
		/* unimplemented SYSCALL */
		default:
			/* what do we do here????? KILL? */
			/* we don't handle these, so pass up or die*/
			/* PassUpOrDie(syscallOld, SYSCALLEXCEPTION); */
			SYSCALL(USERTERMINATE, 0, 0, 0);

	}
}

void UserProgramTrapHandler()
{
	/* bascially just do a sys 18 here */
	SYSCALL(USERTERMINATE, 0, 0, 0);

}

HIDDEN void ReadFromTerminal(int asid, state_PTR syscallOld, char* destAddr){}

HIDDEN void WriteToTerminal(int asid, state_PTR syscallOld, char* sourceAddr, int length)
{
	int i; 
	char * s;
	int deviceNum;
	device_t* deviceAddr;
	unsigned int status;
	int deviceIndex;

	/* if it's a badd address or a bad length, kill them */
	if((int)sourceAddr < LEGALADDRSTART || length < 0)
	{
		/* kill with SYS 18*/
		SYSCALL(USERTERMINATE, 0, 0, 0);
	}
	i = 0; 			/* counter for number of characters transmitted */
	/* char * s = msg; */
	s = sourceAddr;
	/* devregtr * base = (devregtr *) (TERM0ADDR); */
	deviceNum = asid-1;
	deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((TERMINALLINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (deviceNum * DEVICESIZE));
	/* devregtr status; */

	/* SYSCALL(PASSERN, (int)&term_mut, 0, 0); */			/* P(term_mut) */
	deviceIndex = (TERMINALLINE - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + deviceNum;
	SYSCALL(PASSEREN, (int)&deviceSema4s[deviceIndex], 0, 0);
	while (*s != EOS) {
		/* *(base + 3) = PRINTCHR | (((devregtr) *s) << BYTELEN); */
		allowInterrupts(FALSE);
		deviceAddr->t_transm_command = TRANSMITCHAR | (((unsigned int) *s) << DEVICECOMMANDSHIFT);
		/* status = SYSCALL(WAITIO, TERMINT, 0, 0);	*/
		status = SYSCALL(WAITFORIO, TERMINALLINE, deviceNum, FALSE);
		allowInterrupts(TRUE);
		if ((status & TERMINALSTATUSMASK) != CHARTRANSMITTED)
		{
			/* PANIC(); */
			syscallOld->s_v0 = -status;
			SYSCALL(VERHOGEN, (int)&deviceSema4s[deviceIndex], 0, 0);	
			LDST(syscallOld);
		}
		i++;
		s++;
	}
	SYSCALL(VERHOGEN, (int)&deviceSema4s[deviceIndex], 0, 0);				/* V(term_mut) */
	syscallOld->s_v0 = i;
	LDST(syscallOld);
}

/* HIDDEN void DiskPut(state_PTR syscallOld, int* blockAddr, int diskNum, int sector){}

HIDDEN void DiskGet(state_PTR syscallOld, int* blockAddr, int diskNum, int sector){} */

HIDDEN void WriteToPrinter(int asid, state_PTR syscallOld, char* sourceAddr, int length){}

HIDDEN void GetTOD(state_PTR syscallOld)
{
	/* devregarea_t* busReg = (devregarea_t*) RAMBASEADDR;
	syscallOld->s_v0 = busReg->todlo; */
	STCK(syscallOld->s_v0);
	LDST(syscallOld);
}

HIDDEN void UserTerminate(int asid, state_PTR syscallOld)
{
	/* release any mutex we have */
	/* if(userProcArray[asid-1].sema4 < 0)
	{
		SYSCALL(VERHOGEN, (int)&userProcArray[asid-1].sema4, 0, 0);
	} */
	/* V the master sema4 */
	SYSCALL(VERHOGEN, (int)&masterSema4, 0, 0);
	/* terminate */
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}
