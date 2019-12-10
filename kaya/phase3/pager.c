
#include "../h/const.h"
#include "../h/types.h"
#include "../e/initProc.e"
#include "/usr/local/include/umps2/umps/libumps.e"

/* int nextFrame = 0; */
HIDDEN int findFrame();
HIDDEN unsigned int getFrameAddr(unsigned int framePoolStart, int frameNum);


void Pager()
{
	devregarea_t* busReg;
	unsigned int framePoolStart;
	int asid; 
	unsigned int cause;
	int excCode;
	int segment;
	int page;
	int frame;
	/* establish frame pool address */
	busReg = (devregarea_t*) RAMBASEADDR;
	framePoolStart = (busReg->rambase + busReg->ramsize) - ((POOLSIZE + 3) * PAGESIZE);
	/* who am i? */
	asid = (getENTRYHI() && ASIDMASK) >> ASIDSHIFT;	

	/* why am i here? */
	cause = procArray[asid-1].oldAreas[TLBEXCEPTION]->s_cause;
	excCode = cause & EXCCODEMASK >> EXCCODESHIFT;
	/* if it's not an invalid load or store, then just kill it. sorry :( */
	if(excCode != TLBINVALIDLOAD && excCode != TLBINVALIDSTORE)
	{
		SYSCALL(TERMINATEPROCESS, 0, 0, 0);
	}

	/* get page num and segment num */
	segment = (procArray[asid-1].oldAreas[TLBEXCEPTION]->s_HI & SEGMASK) >> SEGSHIFT;
	page = (procArray[asid-1].oldAreas[TLBEXCEPTION]->s_HI & PAGEMASK) >> PAGESHIFT;

	/* gain mutex of swap pool */
	SYSCALL(PASSEREN, &swapmutex, 0, 0);

	/* if it's seg 3 then it might be here already */
	if(segment == KUSEG3)
	{
		pagetbe_t pageTableEntry = kuseg3PT.entries[page];
		valid = (pageTableEntry.entryLo & VALIDMASK) >> VALIDSHIFT;
		/* if the page is already there, then our job is done, so let's quit */
		if(valid)
		{
			SYSCALL(VERHOGEN, &swapmutex, 0, 0);
			LDST(procArray[asid-1].oldAreas[TLBEXCEPTION]);
		}
	}

	/* the page ain't there, so we gotta problem to fix */
	frame = findFrame();
	int deviceIndex;
	device_t* backingStore;

	/* uh oh the frame is occupied... there can only be one of us, so time to give them the boot */
	if(frameSwapPool[frame].ASID != UNOCCUPIEDFRAME)
	{
		int freeloader = frameSwapPool[frame].ASID;
		int segToBoot = frameSwapPool[frame].segNum;
		int pageToBoot = frameSwapPool[frame].pageNum;
		if(segToBoot == KUSEG3)
		{
			/* turn the valid bit off */
			allowInterrupts(FALSE);
			kuseg3PT.entries[pageToBoot].entryLo = kuseg3PT.entries[pageToBoot].entryLo ^ VALIDON;
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
		devIndex = (DISKLINE - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + BACKINGSTORE;
		SYSCALL(PASSEREN, &(deviceSema4s[devIndex]), zero, zero);

		allowInterrupts(FALSE);
		/* write to disk0 */
		backingStore = (device_t*) (BASEDEVICEADDRESS + ((DISKLINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (BACKINGSTORE * DEVICESIZE));
		backingStore->d_command = SEEKCYL + getCylinderNum(pageToBoot) << DEVICECOMMANDSHIFT;
		SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

		backingStore->d_command = WRITEBLK + (getSectorNum(freeloader) << DEVICECOMMANDSHIFT) + (getHeadNum(segToBoot) << 2*DEVICECOMMANDSHIFT);
		backingStore->d_data0 = getFrameAddr(framePoolStart, frame); /* starting addr from where to find stuff to write */
		SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

		allowInterrupts(TRUE);
		/* release mutex of disk0 */
		SYSCALL(VERHOGEN, &(deviceSema4s[devIndex]), zero, zero);		
	} /* done handling if it wasn't empty */

	/* read missing page into selected frame */
	
	/* get mutex of disk0 */
	devIndex = (DISKLINE - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + BACKINGSTORE;
	SYSCALL(PASSEREN, &(deviceSema4s[devIndex]), zero, zero);

	allowInterrupts(FALSE);
	/* write to disk0 */
	backingStore = (device_t*) (BASEDEVICEADDRESS + ((DISKLINE - INITIALDEVLINENUM) * DEVICETYPESIZE) + (BACKINGSTORE * DEVICESIZE));
	backingStore->d_command = SEEKCYL + getCylinderNum(page) << DEVICECOMMANDSHIFT;
	SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

	backingStore->d_command = READBLK + (getSectorNum(asid) << DEVICECOMMANDSHIFT) + (getHeadNum(segment) << 2*DEVICECOMMANDSHIFT);
	backingStore->d_data0 = getFrameAddr(framePoolStart, frame); /* starting addr to write the data retrieved */
	SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

	allowInterrupts(TRUE);
	/* release mutex of disk0 */
	SYSCALL(VERHOGEN, &(deviceSema4s[devIndex]), zero, zero);

	/* update swap pool */
	frameSwapPool[frame].ASID = asid;
	frameSwapPool[frame].segNum = segment;
	frameSwapPool[frame].pageNum = page;

	/* update page table with frame number and valid */
	if(segment == KUSEG3)
	{
		/* turn the valid bit on */
		allowInterrupts(FALSE);
		kuseg3PT.entries[page].entryLo = (frame << FRAMESHIFT) + ((kuseg3PT.entries[pageToBoot].entryLo | VALIDON) & FRAMENUMMASK);
		TLBCLR();
		allowInterrupts(TRUE);
	}
	else if(segment == KUSEG2)
	{
		/* turn the valid bit off */
		allowInterrupts(FALSE);
		userProcArray[asid-1].kuseg2PT.entries[page].entryLo = (frame << FRAMESHIFT) + ((userProcArray[asid-1].kuseg2PT.entries[page].entryLo | VALIDON) & FRAMENUMMASK);
		TLBCLR();
		allowInterrupts(TRUE);
	}
	/* deal with TLB cache consistency */

	/* release mutex */
	SYSCALL(VERHOGEN, &swapmutex, 0, 0);

	/* return control to process */
	LDST(procArray[asid-1].oldAreas[TLBEXCEPTION]);

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
