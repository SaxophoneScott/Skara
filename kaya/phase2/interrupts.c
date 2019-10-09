void InterruptHandler()
{
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;
	unsigned int causeReg = interruptOld->s_cause;
	int i = 0;
	int foundLine = FALSE;
	int lineNum;
	/* find highest priority line with interrupt */
	for(i < NUMLINES && !foundLine)
	{
		unsigned int interruptOn = causeReg & INTERRUPTLINES[i];
		/* line i has an interrupt */
		if(interruptOn != 0)
		{
			foundLine = TRUE;
			lineNum = i;
		}
		i++;
	}
	/* none of the lines had an interrupt... so why are we here? */
	if(!foundLine)
	{
		/* ERROR ERROR */
	}
	else
	{
		/* ignore this one :( */
		if(lineNum == 0)
		{}
		/* it's a clock line ---> coming soon... to a theater near you */
		else if(lineNum == 1 || lineNum == 2)
		{}
		/* it's a device line */
		else
		{
			devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR;
			unsigned int devBitMap = busRegArea->interrupt_dev[lineNum - INITIALDEVLINENUM];
			int j = 0;
			int foundDevice = FALSE;
			int devNum;
			while(j < NUMDEVICES && !foundDevice)
			{
				unsigned int interruptOn = devBitMap & INTERRUPTDEVICES[j];
				/* device i has an interrupt */
				if(interruptOn != 0)
				{
					foundDevice = TRUE;
					devNum = j;
				}
				j++;
			}
			/* none of the devices had an interrupt... so why are we here? */
			if(!foundDevice)
			{
				/* ERROR ERROR */
			}
			else
			{
				device_t* deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (deviceNum * DEVICESIZE));
				
			}
		}
	}
}