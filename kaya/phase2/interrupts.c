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
	cpu_t INTERRUPTSTART;											/* start time of interrupt handler */
	cpu_t INTERRUPTEND;												/* end time of interrupt handler */
	STCK(INTERRUPTSTART);
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;			/* state of the interrupted process */
	unsigned int causeReg = interruptOld->s_cause;
	int i = 0;														/* loop control variable for determining interrupt line number */
	unsigned int lineOn = LINE0;									/* indicates an interrupt on line i */
	int foundLine = FALSE; 											/* true if the interrupting line has been found, false otherwise */
	int lineNum; 													/* highest priority line with interrupt */
	int transmitBool = TRUE; 										/* true if an interrupt on line 7 is transmit, false if receive */
	unsigned int interruptOn;										/* all 0s if there is NOT an interrupt on line i */
	device_t* deviceAddr;											/* address for device with interrupt */
	int index;														/* index of the device's sema4 */
	int * Sema4;													/* device's sema4 */

	/* find highest priority line with interrupt */
	while((i < NUMLINES) && (!foundLine))
	{
		causeReg = causeReg & MASKCAUSEREG; 						/* zero out irrelevant bits */
		interruptOn = causeReg & lineOn; 							/* all 0s if there is NOT an interrupt on line i */

		/* case: line i has an interrupt */
		if(interruptOn != 0x00000000)
		{
			foundLine = TRUE;
			lineNum = i;
		}
		i++; 														/* check the next line */
		lineOn = lineOn << 1;
	}

	/* one of the lines had an interrupt, so let's handle it */
	/* case: it's the proccesor local timer */
	if(lineNum == PLTLINE)
	{
		/* end the current process's turn */
		IncrementProcessTime(currentProcess);
		insertProcQ(&readyQ, currentProcess);
		Scheduler();												/* schedule a new process to run */

	}
	/* case: it's the interval timer */
	if(lineNum == ITLINE)
	{
		int* sema4 = &(semaphoreArray[ITSEMINDEX]);					/* interval timer's semaddr */
		pcb_PTR p;
		/* unblock all the processes that were waiting for the clock */
		while((*sema4) < 0)
		{
			(*sema4)++;
			p = removeBlocked(sema4);
			insertProcQ(&readyQ, p);
		}
		(*sema4) = 0;
		LDIT(INTERVALTIME);
	}
	/* case: it's a device line */
	else
	{
		devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR;
		unsigned int devBitMap = busRegArea->interrupt_dev[lineNum - INITIALDEVLINENUM]; /* dev bit map for the line with an interrupt */
		int j = 0; 													/* loop control variable for determining interrupt device number */
		unsigned int deviceOn = DEVICE0;							/* indicates an interrupt on device i */
		int foundDevice = FALSE; 									/* true if the interrupting device has been found, false otherwise */
		int devNum;													/* highest priority device with interrupt */

		/* find highest priority device with interrupt */
		while(j < NUMDEVICES && !foundDevice)
		{
			devBitMap = devBitMap & MASKDEVBITMAP; 					/* zero out irrelevant bits */
			interruptOn = devBitMap & deviceOn; 					/* all 0s if there is NOT an interrupt on device i */

			/* case: device i has an interrupt */
			if(interruptOn != 0)
			{
				foundDevice = TRUE;
				devNum = j;
			}
			j++; 													/* check the next device */
			deviceOn = deviceOn << 1;
		}

		/* one of the devices had an interrupt, so let's handle it */
		deviceAddr = (device_t*) (BASEDEVICEADDRESS + ((lineNum - INITIALDEVLINENUM) * DEVICETYPESIZE) + (devNum * DEVICESIZE)); 
		index = (lineNum - INITIALDEVLINENUM) * NUMDEVICESPERTYPE + devNum; /* calcuating index */
		/*	deviceAddr = (device_t *)( busRegArea->devreg[index]);  should be the same as the calcuation as before, emphasis on should*/

		/* case: it's a terminal device */
		if(lineNum == TERMINT)
		{
			/* case: transmit is ready, so it's a receive */
			if((deviceAddr->t_transm_status & MASKTRANSCHAR) == READY)
			{
				index += NUMDEVICESPERTYPE; 						/* modify index to get second set of terminal device sema4s (i.e. receive sema4s) */
				transmitBool = FALSE;
			}
			/* else we do nothing as its a trans-status*/
		}

		Sema4 = &(semaphoreArray[index]);
		(*Sema4)++;

		/* case: we need to unblock a process that was waiting for this device */
		if((*Sema4) <= 0)
		{
			pcb_PTR proc = removeBlocked(Sema4);
			/* case: we found one to unblock, so let's return the device's status to it */
			if(proc != NULL)
			{
				/* case: it's a terminal device */
				if(lineNum == TERMINT)
				{
					/* case: it's a transmit */
					if(transmitBool)
					{
						proc->p_s.s_v0 = deviceAddr->t_transm_status; 
					}
					/* case: it's a receive */
					else 
					{
						proc->p_s.s_v0 = deviceAddr->t_recv_status;
					}
				}
				/* case: it's not a terminal device */
				else
				{
					/* SHOULDNT THIS BE D_STATUS */
					proc->p_s.s_v0 = deviceAddr->d_command; /* put this in const*/
				}

				/* the process is now ready */
				softBlockCount--;
				insertProcQ(&readyQ, proc);
			}

			/* now let's acknowledge the interrupt */
			/* case: it's a terminal device */
			if(lineNum == TERMINT)
			{
				/* case: it's a transmit */
				if(transmitBool)
				{
					deviceAddr->t_transm_command = ACK;
				}
				/* case: it's a receive */
				else
				{
					deviceAddr->t_recv_command = ACK;
				}
			}
			/* case: it's not a terminal device */
			else
			{
				deviceAddr->d_command = ACK; /* put this in const*/
			}
		}
	}

	/* we're done here, so let's get another process up and running */
	/* case: there was no process that got interrupted */
	if(currentProcess == NULL)
	{
		Scheduler();
	}											/* schedule a new process to run */
	/* case: a process got interrupted */
	else
	{
		STCK(INTERRUPTEND);
		LoadState(interruptOld); 									/* return control to the interrupted process*/
	}
}
