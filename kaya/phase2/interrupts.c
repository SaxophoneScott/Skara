#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/scheduler.e"

HIDDEN void DetermineLine(state_PTR interruptOld);
HIDDEN void PLTInterrupt();
HIDDEN void ITInterrupt();
HIDDEN void DeviceInterrupt();
HIDDEN void DetermineDevice(devregarea_t busRegArea);
HIDDEN void whatTheHeckIsTheSemaddr(int* semaddr)

void InterruptHandler()
{
	cpu_t INTERRUPTSTART;			/* start time of the interrupt handler */
	cpu_t INTERRUPTEND;				/* end time of the interrupt handler */
	/* STCK(INTERRUPTSTART); */
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;		/* state of the interrupted process */
	int lineNum; 												/* the highest priority line with interrupt */

	/* find highest priority line with interrupt */
	lineNum = determineLine(interruptOld);

	/* one of the lines had an interrupt, so let's handle it */
	/* its the proccesor local timer */
	if(lineNum == 1)
	{
		PLTInterrupt();
	}
	/* its the interval timer */
	if(lineNum == 2)
	{
		ITInterrupt();
	}
	/* it's a device line */
	else
	{
		DeviceInterrupt();
	}
	/* we handled it, so now let's get back to processes */
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

HIDDEN int determineLine(state_PTR interruptOld)
{
	unsigned int causeReg = interruptOld->s_cause;				/* cause register indicating the cause of the interrupt */
	int i = 0;													/* loop control variable for determining interrupt line number */
	unsigned int lineOn = LINE0;								/* variable indicating that there is an interrupt on line i */
	int foundLine = FALSE; 										/* indicates whether or not the interrupt line has been found */
	int lineNum; 												/* the highest priority line with interrupt */
	unsigned int interruptOn;									/* indicates if there is an interrupt on line i */

	while((i < NUMLINES) && (!foundLine))
	{
		causeReg= causeReg & MASKCAUSEREG; 	/* zero out the irrelevant bits */
		interruptOn = causeReg & lineOn; 	/* will be all 0s if there is NOT an interrupt on line i */
		/* line i has an interrupt */
		if(interruptOn != 0x00000000)
		{
			foundLine = TRUE;
			lineNum = i;
		}
		i++; /* check the next line */
		lineOn = lineOn << 1;
	}
	return lineNum;
}

HIDDEN void PLTInterrupt()
{
	IncrementProcessTime(currentProcess);
	insertProcQ(&readyQ, currentProcess);
	Scheduler();
}

HIDDEN void ITInterrupt()
{
	int sema4 = semaphoreArray[SEMCOUNT-1];		/* interval timer sema4 */
	/* unblock all processes blocked on interval timer sema4 */
	whatTheHeckIsTheSemaddr(&sema4);
	while(sema4 < 0)
	{
		sema4++;
		pcb_PTR p = removeBlocked(&sema4);
		insertProcQ(&readyQ, p);
	}
	semaphoreArray[SEMCOUNT-1] = 0;				/* reset the interval timer sema4 */
	LDIT(100000);								/* reload the interval timer */
}

HIDDEN void DeviceInterrupt()
{
	devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR;
	int devNum;							/* the highest priority device with interrupt */
	device_t* deviceAddr;				/* address for the device with the interrupt we are handling */
	int transmitBool = TRUE; 			/* boolean indicating if a line 7 interrupt is transmit or receive */
	int index;							/* index of sema4 for the device we are dealing with */
	int * Sema4;						/* sema4 for the device we are dealing with */

	/* find highest priority device with interrupt */
	devNum = determineDevice(busRegArea);

	/* one of the devices had an interrupt, so let's handle it */
	deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (devNum * DEVICESIZE)); 
	index = (lineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + devNum; /* calcuating index*/
	/*	deviceAddr = (device_t *)( busRegArea->devreg[index]);  should be the same as the calcuation as before, emphasis on should*/
	/* it's a terminal device */
	if(lineNum == TERMINT)
	{
		/* it's a receive not a transmit */
		if((deviceAddr->t_transm_status & MASKTRANSCHAR) == READY)
		{
			index += NUMDEVICESPERTYPE;
			transmitBool = FALSE;
		} 
		/* else we do nothing as it's a transmit */
	}
	Sema4 = &(semaphoreArray[index]);
	(*Sema4)++;
	if((*Sema4) <= 0)
	{
		pcb_PTR proc=removeBlocked(Sema4);
		if(proc!=NULL)
		{
			/* proc->p_s.s_v0 = deviceAddr->d_status; */
			/* it's a terminal device */
			if(lineNum == TERMINT)
			{
				/* it's a transmit, not a receieve */
				if(transmitBool)
				{
					proc->p_s.s_v0 = deviceAddr->t_transm_status; 
				}
				else 
				{
					proc->p_s.s_v0 = deviceAddr->t_recv_status;
				}
			}	
			else
			{
				proc->p_s.s_v0 = deviceAddr->d_command; /* put this in const*/
			}
			softBlockCount--;
			insertProcQ(&readyQ, proc);
		}
		
		if(lineNum == TERMINT)
		{
			if(transmitBool)
			{
				deviceAddr->t_transm_command = ACK; 
			}
			else 
			{
				deviceAddr->t_recv_command = ACK;
			}
		}	
		else
		{
			deviceAddr->d_command = ACK; /* put this in const*/
		}
	}	
}

HIDDEN int determineDevice(devregarea_t* busRegArea)
{
	unsigned int devBitMap = busRegArea->interrupt_dev[lineNum - INITIALDEVLINENUM]; /* dev bit map for the line that has an interrupt */
	int j = 0; 							/* loop control variable for determining interrupt device number */
	unsigned int deviceOn = DEVICE0;	/* variable indicating that there is an interrupt on line i */
	int foundDevice = FALSE; 			/* indicates whether or not the interrupt device has been found */
	int devNum;							/* the highest priority device with interrupt */
	unsigned int interruptOn;			/* indicates if there is an interrupt on device i */

	/* find highest priority device with interrupt */
	while(j < NUMDEVICES && !foundDevice)
	{
		devBitMap = devBitMap & MASKDEVBITMAP; 	/* zero out the irrelevant bits */
		interruptOn = devBitMap & deviceOn; 	/* will be all 0s if there is NOT an interrupt on device i */
		/* device i has an interrupt */
		if(interruptOn != 0)
		{
			foundDevice = TRUE;
			devNum = j;
		}
		j++; /* check the next device */
		deviceOn = deviceOn << 1;
	}
	return devNum;
}

HIDDEN void whatTheHeckIsTheSemaddr(int* semaddr)
{
	z = 0;
	z++;
}
