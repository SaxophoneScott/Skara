void InterruptHandler()
{
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;
	unsigned int causeReg = interruptOld->s_cause;
	int i = 0;				/* loop control variable for determining interrupt line number */
	int foundLine = FALSE; 	/* indicates whether or not the interrupt line has been found */
	int lineNum; 			/* the highest priority line with interrupt */
	int transmitBool = TRUE; /* assuming true because otherwise if its the othercase, we would want to change it to false*/
	
	/* find highest priority line with interrupt */
	while(i < NUMLINES && !foundLine)
	{
		causeReg= causeReg & MASKCAUSEREG; /*  Zero out the irrelevant values */
		unsigned int interruptOn = causeReg & INTERRUPTLINES[i]; /* will be all 0s if there is NOT an interrupt on line i */
		/* line i has an interrupt */
		if(interruptOn != 0)
		{
			foundLine = TRUE;
			lineNum = i;
		}
		i++; /* check the next line */
	}
	
	/* none of the lines had an interrupt... so why are we here? */
	if(!foundLine)
	{
		/* ERROR ERROR */
	}
	/* one of the lines had an interrupt, so let's handle it */
	else
	{
		/* ignore this one... :( */
		if(lineNum == 0)
		{}
		/* it's a clock line ---> coming soon... to a theater near you */
		else if(lineNum == 1 || lineNum == 2)
		{}
		/* it's a device line */
		else
		{
			devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR;
			unsigned int devBitMap = busRegArea->interrupt_dev[lineNum - INITIALDEVLINENUM]; /* dev bit map for the line that has an interrupt */
			int j = 0; 					/* loop control variable for determining interrupt device number */
			int foundDevice = FALSE; 	/* indicates whether or not the interrupt device has been found */
			int devNum;					/* the highest priority device with interrupt */

			/* find highest priority device with interrupt */
			while(j < NUMDEVICES && !foundDevice)
			{
				devBitMap = devBitMap & MASKDEVBITMAP; /* Zeroing out irrelevant bits*/
				unsigned int interruptOn = devBitMap & INTERRUPTDEVICES[j]; /* will be all 0s if there is NOT an interrupt on device i */
				/* device i has an interrupt */
				if(interruptOn != 0)
				{
					foundDevice = TRUE;
					devNum = j;
				}
				j++; /* check the next device */
			}

			/* none of the devices had an interrupt... so why are we here? */
			if(!foundDevice)
			{
				/* ERROR ERROR */
			}
			/* one of the devices had an interrupt, so let's handle it */
			else
			{
				device_t* deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (deviceNum * DEVICESIZE));
				int index = (lineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + deviceNum; /* calcuating index*/
				if(lineNum ==7)
					{
						if(deviceAddr->t_transm_status == 0x00000001)
						{
							index += NUMDEVICESPERTYPE /* its a receive!*/
							transmitBool = FALSE;
						} 
						/* else we do nothing as its a trans-status*/
					}
				int * Sema4 = &(semaphoreArray[index]);
				(*Sema4)++;
				if((*Sema4) <= 0)
					{
						pcb_ptr Proc=removeBlocked(Sema4);
						Proc->p_s->s_v0 = deviceAddr->d_status;
						softBlockCount--;
						insertProcQ(&readyQ, Proc);
						if(lineNum ==7 && transmitBool)
							{
								deviceAddr->d_transm_command = 0x00000001; 
							}	
							else
							{
								deviceAddr->d_command= 0x00000001; /* put this in const*/
							}
						if(currentProcess == NULL)
							{
								Scheduler();  /* want to call scheduler if current process is null*/
							}
						else
							{
								LoadState(interruptOld); /* otherwise we want to return control to the process*/
							}


						



					}


			}
		}
	}
}