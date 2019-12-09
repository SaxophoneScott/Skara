/* int nextFrame = 0; */

void Pager()
{
	/* establish frame pool address */
	devregarea_t* busReg = (devregarea_t*) RAMBASEADDR;
	framePoolStart = (busReg->rambase + busReg->ramsize) - ((POOLSIZE + 3) * PAGESIZE);
	/* who am i? */
	int asid = (getENTRYHI() && ASIDMASK) >> ASIDSHIFT;	

	/* why am i here? */
	unsigned int cause = procArray[asid-1].oldAreas[TLBEXCEPTION]->s_cause;
	int excCode = cause & EXCCODEMASK >> EXCCODESHIFT;
	/* if it's not an invalid load or store, then just kill it. sorry :( */
	if(excCode != TLBINVALIDLOAD && excCode != TLBINVALIDSTORE)
	{
		SYSCALL(TERMINATEPROCESS, 0, 0, 0);
	}

	/* get page num and segment num */
	int segment = (procArray[asid-1].oldAreas[TLBEXCEPTION]->s_HI & SEGMASK) >> SEGSHIFT;
	int page = (procArray[asid-1].oldAreas[TLBEXCEPTION]->s_HI & PAGEMASK) >> PAGESHIFT;

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
	int frame = findFrame();

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
		int diskLineNum = DISKLINE;
		int diskDeviceNum = BACKINGSTORE;
		int devIndex = (diskLineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + diskDeviceNum;
		SYSCALL(PASSEREN, &(deviceSema4s[devIndex]), zero, zero);

		allowInterrupts(FALSE);
		/* write to disk0 */
		device_t* backingStore = (device_t*) (BASEDEVICEADDRESS + ((diskLineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (diskDeviceNum * DEVICESIZE));
		backingStore->d_command = SEEKCYL + getCylinderNum(pageToBoot) << DISKCOMMANDSHIFT;
		SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

		backingStore->d_command = WRITEBLK + (getSectorNum(freeloader) << DISKCOMMANDSHIFT) + (getHeadNum(segToBoot) << 2*DISKCOMMANDSHIFT);
		backingStore->d_data0 = getFrameAddr(framePoolStart, frame); /* starting addr from where to find stuff to write */
		SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

		allowInterrupts(TRUE);
		/* release mutex of disk0 */
		SYSCALL(VERHOGEN, &(deviceSema4s[devIndex]), zero, zero);		
	} /* done handling if it wasn't empty */

	/* read missing page into selected frame */
	
	/* get mutex of disk0 */
	int diskLineNum = DISKLINE;
	int diskDeviceNum = BACKINGSTORE;
	int devIndex = (diskLineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + diskDeviceNum;
	SYSCALL(PASSEREN, &(deviceSema4s[devIndex]), zero, zero);

	allowInterrupts(FALSE);
	/* write to disk0 */
	device_t* backingStore = (device_t*) (BASEDEVICEADDRESS + ((diskLineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (diskDeviceNum * DEVICESIZE));
	backingStore->d_command = SEEKCYL + getCylinderNum(page) << DISKCOMMANDSHIFT;
	SYSCALL(WAITFORIO, diskLineNum, diskDeviceNum, zero);

	backingStore->d_command = READBLK + (getSectorNum(asid) << DISKCOMMANDSHIFT) + (getHeadNum(segment) << 2*DISKCOMMANDSHIFT);
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

int findFrame()
{
	static int nextFrame = 0;
	/* return ((next+1) % POOLSIZE); */
	return ((nextFrame += 1) % POOLSIZE);
}

unsigned int getFrameAddr(unsigned int framePoolStart, int frameNum)
{
	return framePoolStart + (frameNum * PAGESIZE);
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