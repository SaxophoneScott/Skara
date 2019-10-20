#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/scheduler.e"

void InterruptHandler()
{
	cpu_t INTERRUPTSTART;
	cpu_t INTERRUPTEND;
	STCK(INTERRUPTSTART);
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;
	unsigned int causeReg = interruptOld->s_cause;
	int i = 0;						/* loop control variable for determining interrupt line number */
	unsigned int lineOn = LINE0;	/* variable indicating that there is an interrupt on line i */
	int foundLine = FALSE; 			/* indicates whether or not the interrupt line has been found */
	int lineNum; 					/* the highest priority line with interrupt */
	int transmitBool = TRUE; 		/* assuming true because otherwise if its the othercase, we would want to change it to false*/
	unsigned int interruptOn;		/* variable indicating if there is an interrupt on that line */
	device_t* deviceAddr;			
	int index;
	int * Sema4;

	/* find highest priority line with interrupt */
	while((i < NUMLINES) && (!foundLine))
	{
		causeReg= causeReg & MASKCAUSEREG; /*  Zero out the irrelevant values */
		interruptOn = causeReg & lineOn; /* will be all 0s if there is NOT an interrupt on line i */
		/* line i has an interrupt */
		if(interruptOn != 0x00000000)
		{
			foundLine = TRUE;
			lineNum = i;
		}
		i++; /* check the next line */
		lineOn = lineOn << 1;
	}

	/* one of the lines had an interrupt, so let's handle it */
	/* it's a clock line ---> coming soon... to a theater near you */
	/* its the proccesor local timer!*/
	if(lineNum == 1)
	{
		IncrementProcessTime(currentProcess);
		insertProcQ(&readyQ, currentProcess);
		Scheduler();

	}
	/* its the interval timer!*/
	if(lineNum == 2)
	{
		int sema4=semaphoreArray[SEMCOUNT-1];
		while(sema4<0)
		{
			sema4++;
			pcb_PTR p=removeBlocked(&sema4);
			insertProcQ(&readyQ, p);
		}
		semaphoreArray[SEMCOUNT-1]= 0;
		LDIT(100000);


	}
	/* it's a device line */
	else
	{
		devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR;
		unsigned int devBitMap = busRegArea->interrupt_dev[lineNum - INITIALDEVLINENUM]; /* dev bit map for the line that has an interrupt */
		int j = 0; 							/* loop control variable for determining interrupt device number */
		unsigned int deviceOn = DEVICE0;	/* variable indicating that there is an interrupt on line i */
		int foundDevice = FALSE; 			/* indicates whether or not the interrupt device has been found */
		int devNum;							/* the highest priority device with interrupt */

		/* find highest priority device with interrupt */
		while(j < NUMDEVICES && !foundDevice)
		{
			devBitMap = devBitMap & MASKDEVBITMAP; /* Zeroing out irrelevant bits*/
			interruptOn = devBitMap & deviceOn; /* will be all 0s if there is NOT an interrupt on device i */
			/* device i has an interrupt */
			if(interruptOn != 0)
			{
				foundDevice = TRUE;
				devNum = j;
			}
			j++; /* check the next device */
			deviceOn = deviceOn << 1;
		}

		/* one of the devices had an interrupt, so let's handle it */
		deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (devNum * DEVICESIZE)); 
		index = (lineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + devNum; /* calcuating index*/
	/*	deviceAddr = (device_t *)( busRegArea->devreg[index]);  should be the same as the calcuation as before, emphasis on should*/
		if(lineNum == TERMINT)
		{
			if(deviceAddr->t_transm_status == 0x00000001)
			{
				index += NUMDEVICESPERTYPE; /* its a receive!*/
				transmitBool = FALSE;
			} 
			/* else we do nothing as its a trans-status*/
		}
		Sema4 = &(semaphoreArray[index]);
		(*Sema4)++;
		if((*Sema4) <= 0)
		{
			pcb_PTR Proc=removeBlocked(Sema4);
			if(Proc!=NULL)
			{
				Proc->p_s.s_v0 = deviceAddr->d_status;
				softBlockCount--;
				insertProcQ(&readyQ, Proc);
			}
			
			if(lineNum == TERMINT && transmitBool)
			{
				deviceAddr->t_transm_command = 0x00000001; 
			}	
			else
			{
				deviceAddr->d_command= 0x00000001; /* put this in const*/
			}
		}	
	}
	if(currentProcess == NULL)
	{
		Scheduler();  /* want to call scheduler if current process is null*/
	}
	else
	{
		STCK(INTERRUPTEND); 
		LoadState(interruptOld); /* otherwise we want to return control to the process*/
	}
}


