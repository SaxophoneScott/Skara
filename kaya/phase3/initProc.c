
#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/initProc.e"
#include "../e/pager.e"
#include "../e/sysSupport.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* phase 3 global variables */
/* semaphores */
int 						masterSema4;
int 						swapmutex;
int 						deviceSema4s[DEVICECOUNT];
/* structs */
segtable_t* 				segTable;
ospagetable_t				ksegosPT;
kupagetable_t				kuseg3PT;
struct upcb_t				userProcArray[PROCCNT];
struct frameswappoole_t		frameSwapPool[POOLSIZE];

HIDDEN void uProcInit();
HIDDEN void readTape(int procNum);

HIDDEN int debug(int a, int b, int c, int d);

HIDDEN int debug(int a, int b, int c, int d){return a;}

/*
Method to setup everything necessary to phase 3 VM support and user process execution.
Initializes all phase 3 global variables and sets up all user processes.
*/
void test()
{
	int i;								/* loop control variable */
	int n;								/* loop control variable */
	state_t initialState;				/* state for process that will initialize user processes */

	/* initializing all phase 3 globals */

	/* initialize segment table location */
	segTable = (segtable_t*) SEGMENTTABLE;								/* needs to go at 0x20000500 so that it can be found */

	/* initialize ksegOS page table */
	ksegosPT.header = (MAGICNUM << MAGICNUMSHIFT) | MAXKSEGOS;
	/* initialize entry for each page */
	for(i = 0; i < MAXKSEGOS; i++)
	{
		/* entryHi = 0x20000 + i (ASID is irrelephant) */
		ksegosPT.entries[i].entryHi = (KSEGOSSTART + i) << PAGESHIFT;
		/* entryLo = 0x20000 + i with dirty, valid, global */
		ksegosPT.entries[i].entryLo = ((KSEGOSSTART + i) << PAGESHIFT) | DIRTYON | VALIDON | GLOBALON;
		/* debug((int)ksegosPT.entries[i].entryHi, (int)ksegosPT.entries[i].entryLo,0,0); */
	}
	devregarea_t* busReg = (devregarea_t*) RAMBASEADDR;
	memaddr ramtop = busReg->rambase + busReg->ramsize;
	memaddr ptBottom = &ksegosPT;
	memaddr ptTop = ptBottom + 4 + (8 * MAXKSEGOS);
	debug((int)ramtop, (int)ptBottom, (int)ptTop,0);

	/* initialize kuseg3 page table */
	kuseg3PT.header = (MAGICNUM << MAGICNUMSHIFT) | MAXKUSEG;
	/* initialize entry for each page */
	for(i = 0; i < MAXKUSEG; i++)
	{
		/* entryHi = 0xC0000 + i (ASID is irrelephant again since this is a shared segment) */
		kuseg3PT.entries[i].entryHi = (KUSEG3START + i) << PAGESHIFT;
		/* entryLo = dirty, global */
		kuseg3PT.entries[i].entryLo = ALLOFF | DIRTYON | GLOBALON;
	}
	ptBottom = &kuseg3PT;
	ptTop = ptBottom + 4 + (8 * MAXKUSEG);
	debug((int)ramtop, (int)ptBottom, (int)ptTop,0);

	/* initialize swap pool entries */
	for(i = 0; i < POOLSIZE; i++)
	{
		/* ASID = -1 to indicate unoccupied */
		frameSwapPool[i].ASID = UNOCCUPIEDFRAME;
	}

	/* initialize swap pool semaphore for mutual exclusion */
	swapmutex = MUTEXINIT;									/* = 1 (mutex) */

	/* initialize all device sema4s for mutual exclusion */
	for(i = 0; i < DEVICECOUNT; i++)
	{
		deviceSema4s[i] = MUTEXINIT;						/* = 1 (mutex) */
	}

	/* initialize master semaphore for synchronization
	/* used for graceful process termination to know when all processes are done and it should SYS2 itself */
	masterSema4 = SYNCINIT; 								/* = 0 (sync) */

	/* initialize all user processes including their page table and segment table entry */
	for(n = 1; n < PROCCNT + 1; n++)
	{
		/* initialize kuseg2 page table for proc n */
		userProcArray[n-1].kuseg2PT.header = (MAGICNUM << MAGICNUMSHIFT) | MAXKUSEG;
		/* initialize entry for each page */
		for(i = 0; i < MAXKUSEG; i++)
		{
			/* entryHi = 0x80000 + i with ASID n */
			userProcArray[n-1].kuseg2PT.entries[i].entryHi = ((KUSEG2START + i) << PAGESHIFT) | (n << ASIDSHIFT);
			/* entryLo = no frame #, dirty, not valid, not global */
			userProcArray[n-1].kuseg2PT.entries[i].entryLo = ALLOFF | DIRTYON;
		}
		/* fix last entry's entryHi to be 0xBFFFF w/ ASID n since this is the stack page */
		userProcArray[n-1].kuseg2PT.entries[MAXKUSEG-1].entryHi = (KUSEG2LAST << PAGESHIFT) | (n << ASIDSHIFT);

		ptBottom = &(userProcArray[n-1].kuseg2PT);
		ptTop = ptBottom + 4 + (8 * MAXKUSEG);
		debug((int)ramtop, (int)ptBottom, (int)ptTop,0);

		/* setup appropriate 3 entries (for proc n) in global segment table */
		segTable->entries[n].ksegos = &ksegosPT; 							/* ksegOS = global var table */
		segTable->entries[n].kuseg2 = &(userProcArray[n-1].kuseg2PT);		/* kuseg2 = process's specific table we just set up */
		segTable->entries[n].kuseg3 = &kuseg3PT;							/* kuseg3 = global var table */
		debug((int)segTable, (int)segTable->entries[n-1].ksegos, (int)segTable->entries[n-1].kuseg2, (int)segTable->entries[n-1].kuseg3);

		/* setup initial process state that will complete user process setup */
		initialState.s_asid = n << ASIDSHIFT;
		initialState.s_sp = (memaddr) getStackPageAddr(n, 0);				/* can use any of the process's stack pages */
		initialState.s_pc = (memaddr) uProcInit;							/* method to set up trap handlers for proc n and read in proc n's .text and .data */
		initialState.s_t9 = (memaddr) uProcInit;
		initialState.s_status = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | INITVMOFF | KERNELON;

		/* SYS 1 */
		SYSCALL(CREATEPROCESS, (int)&initialState, 0, 0);
	}

	/* P the master semaphore for each process created so we know to keep running until they are all terminated */
	for(i = 0; i < PROCCNT; i++)
	{
		SYSCALL(PASSEREN, (int)&masterSema4, 0, 0);
	}

	debug(0,0,0,0);
	/* all user processes have terminated, so we can terminate now */
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/*
Method to initialize the user process.
Sets up three new state for the different trap handlers, reads the user process's .text and .data onto the
backing store device, sets up the processor state to be used for the user process's execution, and loads
this processor state.
*/
HIDDEN void uProcInit()
{
	int asid;										/* the user process's ASID */
	state_t initialState;							/* the initial state for the user process */
	/* status for the trap handlers */
	unsigned int newStateStatus = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | VMONPREV | KERNELON;

	/* who am i? */
	asid = (getENTRYHI() & ASIDMASK) >> ASIDSHIFT;

	/* set up 3 new areas for pass up or die */
	/* TLB */
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_asid = asid << ASIDSHIFT;
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_sp = (memaddr) getStackPageAddr(asid, TLBEXCEPTION);
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_pc = (memaddr) UserTLBHandler;
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_t9 = (memaddr) UserTLBHandler;
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_status = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | KERNELON;
	/* Program Trap */
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_asid = asid << ASIDSHIFT;
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_sp = (memaddr) getStackPageAddr(asid, PROGRAMTRAPEXCEPTION);
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_pc = (memaddr) UserProgramTrapHandler;
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_t9 = (memaddr) UserProgramTrapHandler;
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_status = newStateStatus;
	/* Syscall */
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_asid = asid << ASIDSHIFT;
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_sp = (memaddr) getStackPageAddr(asid, SYSCALLEXCEPTION);
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_pc = (memaddr) UserSyscallHandler;
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_t9 = (memaddr) UserSyscallHandler;
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_status = newStateStatus;

	/* 3 SYS5s to actually designate our own handlers */
	SYSCALL(EXCEPTIONSTATEVEC, TLBEXCEPTION, (int)&(userProcArray[asid-1].oldAreas[TLBEXCEPTION]), (int)&(userProcArray[asid-1].newAreas[TLBEXCEPTION]));
	SYSCALL(EXCEPTIONSTATEVEC, PROGRAMTRAPEXCEPTION, (int)&(userProcArray[asid-1].oldAreas[PROGRAMTRAPEXCEPTION]), (int)&(userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION]));
	SYSCALL(EXCEPTIONSTATEVEC, SYSCALLEXCEPTION, (int)&(userProcArray[asid-1].oldAreas[SYSCALLEXCEPTION]), (int)&(userProcArray[asid-1].newAreas[SYSCALLEXCEPTION]));

	/* read in the user process's .text and .data */
	readTape(asid);

	/* set up user process's initial state */
	initialState.s_asid = asid << ASIDSHIFT;
	initialState.s_sp = (memaddr) LASTPAGEKUSEG2;									/* stack page is the last page of kuseg2 */
	initialState.s_status = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | KERNELOFF | VMONPREV;
	initialState.s_pc = (memaddr) UPROCPCINIT;										/* pc is the second word of kuseg2 */
	initialState.s_t9 = (memaddr) UPROCPCINIT;

	debug((int)initialState.s_sp,(int)initialState.s_status,(int)initialState.s_pc,0);
	LDST(&initialState);												/* start the user process up and running */
}

/*
Method to read the user process's .text and .data from the tape and put it onto the backing store device.
Reads a block from the tape associated with the user process and places that block onto backing store
repeatedly. Continues this until there is nothing left on the tape.
param:procNum - the ASID of the user process
*/
HIDDEN void readTape(int procNum)
{
	int i = 0;														/* counter for page number */
	int moreToRead = TRUE;											/* flag for if there is more to read off the tape */
	unsigned int tapeStatus;										/* the status of the tape device */
	int tapeDeviceNum = procNum-1;									/* device num for the device associated with the user process */
	int zero = 0;
	/* pointer to the tape's device register */
	device_t* tape = (device_t*) (BASEDEVICEADDRESS + ((TAPELINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (tapeDeviceNum * DEVICESIZE));

	/* read a block from the tape atomically */
	allowInterrupts(FALSE);
	tape->d_data0 = getTapeBufferAddr(tapeDeviceNum); 				/* starting address of where we want to put the tape data */
	tape->d_command = READBLK;
	tapeStatus = SYSCALL(WAITFORIO, TAPELINE, tapeDeviceNum, zero);
	allowInterrupts(TRUE);

	while(tapeStatus == READY && moreToRead)
	{
		device_t* backingStore;
		/* get mutex of disk0 */
		int devIndex = (DISKLINE - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + BACKINGSTORE;
		SYSCALL(PASSEREN, (int)&(deviceSema4s[devIndex]), zero, zero);

		allowInterrupts(FALSE);
		/* write to disk0 */
		backingStore = (device_t*) (BASEDEVICEADDRESS + ((DISKLINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (BACKINGSTORE * DEVICESIZE));
		backingStore->d_command = SEEKCYL + (getCylinderNum(i) << DEVICECOMMANDSHIFT);
		SYSCALL(WAITFORIO, DISKLINE, BACKINGSTORE, zero);

		backingStore->d_data0 = getTapeBufferAddr(tapeDeviceNum); /* getDiskBufferAddr(BACKINGSTORE); */ /* starting addr from where to find stuff to write */
		backingStore->d_command = WRITEBLK + (getSectorNum(procNum) << DEVICECOMMANDSHIFT) + (getHeadNum(KUSEG2) << 2*DEVICECOMMANDSHIFT);
		SYSCALL(WAITFORIO, DISKLINE, BACKINGSTORE, zero);

		allowInterrupts(TRUE);
		/* release mutex of disk0 */
		SYSCALL(VERHOGEN, (int)&(deviceSema4s[devIndex]), zero, zero);

		moreToRead = (tape->d_data1 == EOB); /* (tape->d_data1 == EOB); */
		/* debug(getCylinderNum(i), getSectorNum(procNum), getHeadNum(KUSEG2), moreToRead); */
		i++;

		if(moreToRead)
		{
			allowInterrupts(FALSE);
			/* read the tape */
			tape->d_data0 = getTapeBufferAddr(tapeDeviceNum); /* put starting address of where we wanna put the tape data */
			tape->d_command = READBLK;
			tapeStatus = SYSCALL(WAITFORIO, TAPELINE, tapeDeviceNum, zero);
			allowInterrupts(TRUE);
		}

	}
}

void allowInterrupts(int on)
{
	if(on)
	{
		setSTATUS(getSTATUS() | ENABLEINTERRUPTS);
	}
	else
	{
		setSTATUS(getSTATUS() & DISABLEINTERRUPTS);
	}
}

unsigned int getTapeBufferAddr(int tapeNum)
{
	return TAPEBUFFERSTART + (tapeNum * PAGESIZE);
}

unsigned int getDiskBufferAddr(int diskNum)
{
	return DISKBUFFERSTART + (diskNum * PAGESIZE);
}

unsigned int getStackPageAddr(int procNum, int exceptionType)
{
	return STACKPOOLSTART + ((procNum-1) * UPROCSTACKSIZE * PAGESIZE) + (exceptionType * PAGESIZE) + PAGESIZE;
}

int getCylinderNum(int pageNum)
{
	if(pageNum >= 31)
	{
		return 31;
	}
	else
	{
		return pageNum;
	}
}

int getSectorNum(int procNum)
{
	return procNum-1;
}

int getHeadNum(int segment)
{
	if(segment == KUSEG2)
	{
		return KUSEG2HEAD;
	}
	else if(segment == KUSEG3)
	{
		return KUSEG3HEAD;
	}
}
