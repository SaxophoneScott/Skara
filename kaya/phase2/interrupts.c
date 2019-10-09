void InterruptHandler()
{
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;
	unsigned int causeReg = interruptOld->s_cause;
	int i = 0;				/* loop control variable for determining interrupt line number */
	int foundLine = FALSE; 	/* indicates whether or not the interrupt line has been found */
	int lineNum; 			/* the highest priority line with interrupt */
	
	/* find highest priority line with interrupt */
	while(i < NUMLINES && !foundLine)
	{
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

			}
		}
	}
}