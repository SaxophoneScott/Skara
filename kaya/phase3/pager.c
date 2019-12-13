
#include "../h/const.h"
#include "../h/types.h"
#include "../e/initProc.e"
#include "../e/pager.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* int nextFrame = 0; */
HIDDEN int findFrame();
HIDDEN unsigned int getFrameAddr(unsigned int framePoolStart, int frameNum);

HIDDEN int debug2(int a, int b, int c, int d);
HIDDEN int debug2(int a, int b, int c, int d){ return a; }

void UserTLBHandler()
{
	/* setSTATUS(ALLOFF | INTERRUPTSUNMASKED | INTERRUPTMASKON | TEBITON | VMONPREV | KERNELON); */
	devregarea_t* busReg;
	unsigned int framePoolStart;
	int asid;
	unsigned int cause;
	int excCode;
	int segment;
	int page;
	int frame;
	unsigned int frameAddr;

	/* establish frame pool address */
	busReg = (devregarea_t*) RAMBASEADDR;
	framePoolStart = (busReg->rambase + busReg->ramsize) - ((POOLSIZE + 3) * PAGESIZE);
	/* who am i? */
	asid = (getENTRYHI() & ASIDMASK) >> ASIDSHIFT;

	/* why am i here? */
	cause = userProcArray[asid-1].oldAreas[TLBEXCEPTION].s_cause;
	excCode = (cause & EXCCODEMASK) >> EXCCODESHIFT;
	debug2((int)cause,excCode,0,0);

	/* if it's not an invalid load or store, then just kill it. sorry :( */
	if((excCode != TLBINVALIDLOAD) && (excCode != TLBINVALIDSTORE))
	{
		SYSCALL(USERTERMINATE, 0, 0, 0);
	}

	/* get page num and segment num */
	segment = (userProcArray[asid-1].oldAreas[TLBEXCEPTION].s_asid & SEGMASK) >> SEGSHIFT;
	page = (userProcArray[asid-1].oldAreas[TLBEXCEPTION].s_asid & PAGEMASK) >> PAGESHIFT;
	debug2((int)segment, (int)page,0,0);

	/* gain mutex of swap pool */
	SYSCALL(PASSEREN, (int)&swapmutex, 0, 0);

	/* if it's seg 3 then it might be here already */
	if(segment == KUSEG3)
	{
		debug2(3,3,3,3);
		pagetbe_t pageTableEntry = segTable->entries[asid-1].kuseg3->entries[page];
		unsigned int valid = (pageTableEntry.entryLo & VALIDMASK) >> VALIDSHIFT;
		/* if the page is already there, then our job is done, so let's quit */
		if(valid)
		{
			SYSCALL(VERHOGEN, (int)&swapmutex, 0, 0);
			LDST(&(userProcArray[asid-1].oldAreas[TLBEXCEPTION]));
		}
	}

	/* the page ain't there, so we gotta problem to fix */
	int deviceIndex;
	device_t* backingStore;
	frame = findFrame();
	frameAddr = getFrameAddr(framePoolStart, frame);
	debug2(frame, (int)frameAddr,(int)framePoolStart,0);

	/* uh oh the frame is occupied... there can only be one of us, so time to give them the boot */
	if(frameSwapPool[frame].ASID != UNOCCUPIEDFRAME)
	{
		debug2(-1,-1,-1,-1);
		int freeloader = frameSwapPool[frame].ASID;
		int segToBoot = frameSwapPool[frame].segNum;
		int pageToBoot = frameSwapPool[frame].pageNum;
		if(segToBoot == KUSEG3)
		{
			/* turn the valid bit off */
			allowInterrupts(FALSE);
			segTable->entries[freeloader].kuseg3->entries[pageToBoot].entryLo = segTable->entries[freeloader].kuseg3->entries[pageToBoot].entryLo ^ VALIDON;
			TLBCLR();
			allowInterrupts(TRUE);
		}
		else if(segToBoot == KUSEG2)
		{
			/* turn the valid bit off */
			allowInterrupts(FALSE);
			userProcArray[freeloader-1].kuseg2PT.entries[pageToBoot].entryLo = userProcArray[freeloader-1].kuseg2PT.entries[pageToBoot].entryLo ^ VALIDON;
			TLBCLR();
			allowInterrupts(TRUE);
		}

		/* assume the page is dirty, so write it to backing store */
		/* get mutex of disk0 */
		deviceIndex = (DISKLINE - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + BACKINGSTORE;
		SYSCALL(PASSEREN, (int)&(deviceSema4s[deviceIndex]), 0, 0);

		allowInterrupts(FALSE);
		/* write to disk0 */
		backingStore = (device_t*) (BASEDEVICEADDRESS + ((DISKLINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (BACKINGSTORE * DEVICESIZE));
		backingStore->d_command = SEEKCYL + (getCylinderNum(pageToBoot) << DEVICECOMMANDSHIFT);
		SYSCALL(WAITFORIO, DISKLINE, BACKINGSTORE, 0);

		backingStore->d_data0 = frameAddr; /* starting addr from where to find stuff to write */
		backingStore->d_command = WRITEBLK + (getSectorNum(freeloader) << DEVICECOMMANDSHIFT) + (getHeadNum(segToBoot) << 2*DEVICECOMMANDSHIFT);
		SYSCALL(WAITFORIO, DISKLINE, BACKINGSTORE, 0);

		allowInterrupts(TRUE);
		/* release mutex of disk0 */
		SYSCALL(VERHOGEN, (int)&(deviceSema4s[deviceIndex]), 0, 0);
	} /* done handling if it wasn't empty */

	debug2(0,0,0,0);
	/* read missing page into selected frame */

	/* get mutex of disk0 */
	deviceIndex = (DISKLINE - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + BACKINGSTORE;
	SYSCALL(PASSEREN, (int)&(deviceSema4s[deviceIndex]), 0, 0);

	allowInterrupts(FALSE);
	/* write to disk0 */
	backingStore = (device_t*) (BASEDEVICEADDRESS + ((DISKLINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (BACKINGSTORE * DEVICESIZE));
	backingStore->d_command = SEEKCYL + (getCylinderNum(page) << DEVICECOMMANDSHIFT);
	SYSCALL(WAITFORIO, DISKLINE, BACKINGSTORE, 0);

	backingStore->d_data0 = frameAddr; /* starting addr to write the data retrieved */
	backingStore->d_command = READBLK + (getSectorNum(asid) << DEVICECOMMANDSHIFT) + (getHeadNum(segment) << 2*DEVICECOMMANDSHIFT);
	SYSCALL(WAITFORIO, DISKLINE, BACKINGSTORE, 0);

	allowInterrupts(TRUE);
	/* release mutex of disk0 */
	SYSCALL(VERHOGEN, (int)&(deviceSema4s[deviceIndex]), 0, 0);

	/* update swap pool */
	frameSwapPool[frame].ASID = asid;
	frameSwapPool[frame].segNum = segment;
	frameSwapPool[frame].pageNum = page;

	debug2(2,2,2,2);
	/* update page table with frame number and valid */
	if(segment == KUSEG3)
	{
		/* turn the valid bit on */
		allowInterrupts(FALSE);
		segTable->entries[asid].kuseg3->entries[page].entryLo = (frameAddr) + ((segTable->entries[asid].kuseg3->entries[page].entryLo | VALIDON) & FRAMENUMMASK);
		TLBCLR();
		allowInterrupts(TRUE);
	}
	else if(segment == KUSEG2)
	{
		/* turn the valid bit off */
		allowInterrupts(FALSE);
		userProcArray[asid-1].kuseg2PT.entries[page].entryLo = (frameAddr) + ((userProcArray[asid-1].kuseg2PT.entries[page].entryLo | VALIDON) & FRAMENUMMASK);
		TLBCLR();
		allowInterrupts(TRUE);
	}
	/* deal with TLB cache consistency */

	/* release mutex */
	SYSCALL(VERHOGEN, (int)&swapmutex, 0, 0);

	debug2(42,42,42,42);
	/* return control to process */
	LDST(&(userProcArray[asid-1].oldAreas[TLBEXCEPTION]));

}

HIDDEN int findFrame()
{
	static int nextFrame = 0;
	/* return ((next+1) % POOLSIZE); */
	return ((nextFrame += 1) % POOLSIZE);
}

HIDDEN unsigned int getFrameAddr(unsigned int framePoolStart, int frameNum)
{
	return framePoolStart + (frameNum * PAGESIZE);
}
