
#include "../h/const.h"
#include "../h/types.h"
#include "../e/initial.e"
#include "../e/initProc.e"
#include "../e/pager.e"
#include "../e/sysSupport.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* phase 3 global variables */
/* semaphores */
int 					masterSema4;
int 					swapmutex;
int 					deviceSema4s[DEVICECOUNT];
/* structs */
segtable_t* 			segTable;
ospagetable_t			ksegosPT;
kupagetable_t			kuseg3PT;
struct upcb_t					userProcArray[PROCCNT];
struct frameswappoole_t		frameSwapPool[POOLSIZE];

HIDDEN void uProcInit();
HIDDEN void readTape(int procNum);

HIDDEN int debug(int a, int b, int c, int d);

HIDDEN int debug(int a, int b, int c, int d){return a;}

void test()
{
	int i;								/* loop control variable for various initializations */
	int n;								/* loop control variable for various initializations */
	state_t initialState;

	/* initializing all phase 3 globals */
	segTable = (memaddr) SEGMENTTABLE;

	/* ksegOS page table */
	ksegosPT.header = (MAGICNUM << MAGICNUMSHIFT) + MAXKSEGOS;

	for(i = 0; i < MAXKSEGOS; i++)
	{
		/* entryHi = 0x20000 + i (ASID is irrelephant) */
		ksegosPT.entries[i].entryHi = (KSEGOSSTART + i) << PAGESHIFT;
		/* entryLo = 0x20000 + i with dirty, valid, global */
		ksegosPT.entries[i].entryLo = (KSEGOSSTART + i) | DIRTYON | VALIDON | GLOBALON;
	}

	/* init kuseg3 page table */
	kuseg3PT.header = (MAGICNUM << MAGICNUMSHIFT) + MAXKUSEG;
	for(i = 0; i < MAXKUSEG; i++)
	{
		/* entryHi = 0xC0000 + i */
		kuseg3PT.entries[i].entryHi = (KUSEG3START + i) << PAGESHIFT;
		/* entryLo = dirty, global */
		kuseg3PT.entries[i].entryLo = ALLOFF | DIRTYON | GLOBALON;
	}

	/* swap pool */
	for(i = 0; i < POOLSIZE; i++)
	{
		/* ASID = -1 to indicate unoccupied */
		frameSwapPool[i].ASID = UNOCCUPIEDFRAME;
	}

	swapmutex = MUTEXINIT;

	/* device sema4s */
	for(i = 0; i < DEVICECOUNT; i++)
	{
		/* = 1 (mutex) */
		deviceSema4s[i] = MUTEXINIT;
	}

	masterSema4 = SYNCINIT; /* (to know when all processes are done and it should SYS2 itself) */

	for(n = 1; n < PROCCNT+ 1; n++)
	{
		/* setup proc n's  kuseg2 page table */
		userProcArray[n-1].kuseg2PT.header = (MAGICNUM << MAGICNUMSHIFT) + MAXKUSEG;
		for(i = 0; i < MAXKUSEG; i++)
		{
			/* entryHi = 0x80000 + i with ASID n */
			userProcArray[n-1].kuseg2PT.entries[i].entryHi = ((KUSEG2START + i) << PAGESHIFT) + (n << ASIDSHIFT);  /* should this be +i or +n??? */
			/* entryLo = no frame #, dirty, not valid, not global */
			userProcArray[n-1].kuseg2PT.entries[i].entryLo = ALLOFF | DIRTYON;
		}
		/* fix last entry's entryHi to be 0xBFFFF w/ ASID n */
		userProcArray[PROCCNT-1].kuseg2PT.entries[MAXKUSEG-1].entryHi = (KUSEG2LAST << PAGESHIFT) + (n << ASIDSHIFT);

		/* setup appropriate 3 entries (for proc n) in global segment table */
			/* ksegOS = global var table
			   kuseg2 = process's table we just set up
			   kuseg3 = global var table
			*/
		segTable->entries[n-1].ksegos = &ksegosPT;
		segTable->entries[n-1].kuseg2 = &(userProcArray[n-1].kuseg2PT);
		segTable->entries[n-1].kuseg3 = &kuseg3PT;

		/* setup initial process state */
			/* ASID = n
			   stack page = TBD
			   pc = uProcInit method
			   status = all interrupts enabled, PLT enabled, VM off, kernel mode on
			*/
		initialState.s_asid = n;
		initialState.s_sp = (memaddr) getStackPageAddr(n, 0);		/* can use any of the process's stack pages for now */
		initialState.s_pc = (memaddr) uProcInit;
		initialState.s_t9 = (memaddr) uProcInit;
		initialState.s_status = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | INITVMOFF | KERNELON;

		/* sys 1 */
		SYSCALL(CREATEPROCESS, (int)&initialState, 0, 0);
	}

	for(i = 0; i < PROCCNT; i++)
	{
		SYSCALL(PASSEREN, (int)&masterSema4, 0, 0);
	}

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

HIDDEN void uProcInit()
{
	/* init kuseg2 page table
		3 sys 5s
		read code from tape to backing store
		LDST */
	int asid;
	/* state_PTR tlbNewArea;
	state_PTR progNewArea;
	state_PTR sysNewArea; */
	state_t initialState;
	unsigned int newStateStatus = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | VMON | KERNELON;
	/* who am i? */
	asid = (getENTRYHI() && ASIDMASK) >> ASIDSHIFT;

	/* set up 3 new areas for pass up or die */
	/* TLB */
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_asid = asid;
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_sp = getStackPageAddr(asid, TLBEXCEPTION);
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_pc = (memaddr) Pager;
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_t9 = (memaddr) Pager;
	userProcArray[asid-1].newAreas[TLBEXCEPTION].s_status = newStateStatus;
	/* program trap */
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_asid = asid;
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_sp = getStackPageAddr(asid, PROGRAMTRAPEXCEPTION);
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_pc = (memaddr) UserProgramTrapHandler;
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_t9 = (memaddr) UserProgramTrapHandler;
	userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION].s_status = newStateStatus;
	/* syscall */
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_asid = asid;
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_sp = getStackPageAddr(asid, SYSCALLEXCEPTION);
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_pc = (memaddr) UserSyscallHandler;
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_t9 = (memaddr) UserSyscallHandler;
	userProcArray[asid-1].newAreas[SYSCALLEXCEPTION].s_status = newStateStatus;

	/* 3 SYS5s */
	SYSCALL(EXCEPTIONSTATEVEC, TLBEXCEPTION, (int)&(userProcArray[asid-1].oldAreas[TLBEXCEPTION]), (int)&(userProcArray[asid-1].newAreas[TLBEXCEPTION]));
	SYSCALL(EXCEPTIONSTATEVEC, PROGRAMTRAPEXCEPTION, (int)&(userProcArray[asid-1].oldAreas[PROGRAMTRAPEXCEPTION]), (int)&(userProcArray[asid-1].newAreas[PROGRAMTRAPEXCEPTION]));
	SYSCALL(EXCEPTIONSTATEVEC, SYSCALLEXCEPTION, (int)&(userProcArray[asid-1].oldAreas[SYSCALLEXCEPTION]), (int)&(userProcArray[asid-1].newAreas[SYSCALLEXCEPTION]));

	readTape(asid);

	/* set up user process state */
	initialState.s_asid = asid;
	initialState.s_sp = LASTPAGEKUSEG2;
	initialState.s_status = ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | VMON | KERNELOFF;
	initialState.s_pc = UPROCPCINIT;
	initialState.s_t9 = UPROCPCINIT;

	LDST(&initialState);
}

HIDDEN void readTape(int procNum)
{
	int i = 0;							/* counter for page number */
	int moreToRead = TRUE;
	unsigned int tapeStatus;
	int tapeDeviceNum = procNum-1;
	int zero = 0;
	device_t* tape = (device_t*) (BASEDEVICEADDRESS + ((TAPELINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (tapeDeviceNum * DEVICESIZE));

	allowInterrupts(FALSE);
	/* read the tape */
	tape->d_data0 = getTapeBufferAddr(tapeDeviceNum); /* put starting address of where we wanna put the tape data */
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

		moreToRead = (tape->d_data1 == EOB);
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
