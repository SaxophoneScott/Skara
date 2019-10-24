/************************************************************* Interrupts.c *********************************************************************
* written by Scott Harrington and Kara Schatz                      																				
* 																																				
*																					
* 	Purpose: Implements an interrupt handler that has the capability to handle 3 main types of interrupts:
*		1.) Processor Local Timer interrupts (PLT)
*		2.) Interval Timer interrupts (IT)
*		3.) Device interrupts (both terminal and non-terminal)
* 
*	The interrupt handler gets invoked by the O.S. whenever a previously initiated I/O request completes or the PLT or IT makes
* 	a 0x00000000 to 0xFFFFFFFF transition. The address of the interrupt handler in placed in the new interrupt processor state
*	area so that control will immediately be passed to it when an interrupt occurs. 
*  	
* 	The one interrupt is handled at a time, so to determine which interrupt should be handled, the cause register in the old 
* 	interrupt processor state area is examined. Whichever interrupt line with an current pending interrupt is of the highest 
* 	priority is the one that gets handled.
*
* 	A Processor Local Timer interrupt occurs when the PLT makes a 0x00000000 to 0xFFFFFFFF transition. Handling of a PLT 
* 	interrupt entails ending the "turn" of the interrupted process by placing it on the O.S.'s ready queue, and allowing 
* 	another process to have a "turn" running by invoking the O.S.'s scheduler. The interrupt handler also maintains accumulated 
* 	CPU time usage by updated this whenever a process is ended via a PLT interrupt.
*
* 	An Interval Timer interrupt occurs when the IT makes a 0x00000000 to 0xFFFFFFFF transition. Handling of an IT interrupt 
* 	entails unblocking all processes that executed a Syscall 7 and were waiting for the clock. These processes are now ready, 
* 	so they are placed on the O.S.'s ready queue. The Interval Timer is then reloaded with 100 milliseconds. 
*
* 	Handling of a Device interrupt entails first determining which device we need to handle. Similar to determining the line 
* 	to handle, we look at the device bit map for the appropriate line, and then we determine the highest priority device on that
* 	line that has a pending interrupt. Note that in the case of a terminal device, which is actually 2 sub-devices, an interrupt
* 	on transmit is of higher priority than an interrupt on receipt. Next a process which has requested I/O from this device is 
* 	unblocked, and the device status is returned to it via its v0 register. Finally, the interrupt is acknowledged by placing
* 	writing the ACK command into the deivce's command field.
* 																																														
*	interrupts.c includes types.h, const.h, pcb.e, asl.e, exceptions.e, initial.e, interrupts.e, scheduler.e, 
* 	and the umps2 library.
* 	interrupts.c requires the following phase 2 global variables: ready queue, current process, softblock count, semaphore array                    																																																										*
*
***********************************************************************************************************************************************/



#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/initial.e"
#include "../e/interrupts.e"
#include "../e/scheduler.e"

HIDDEN int DetermineLine(unsigned int causeReg);
HIDDEN int DetermineDevice(unsigned int devBitMap);
HIDDEN void PLTInterruptHandler();
HIDDEN void ITInterruptHandler();
HIDDEN void ExitInterruptHandler(state_PTR interruptOld);

void InterruptHandler()
/* Manages all processing required in order to handle any of the 3 types of possible interrupts:
Processor Local Timer, Interval Timer, or Device interrupt.
Determines the highest priority interrupt to handle, handles it, and then returns control back to
the interrupted process. The scheduler is invoked if there was no process running immediately
before the interrupt occurred. */
{
	state_PTR interruptOld = (state_PTR) INTERRUPTOLDAREA;			/* state of the interrupted process */
	unsigned int causeReg = interruptOld->s_cause;
	int lineNum; 													/* highest priority line with interrupt */

	lineNum = DetermineLine(causeReg);

	/* one of the lines had an interrupt, so let's handle it */
	/* case: it's the proccesor local timer */
	if(lineNum == PLTLINE)
	{
		PLTInterruptHandler();
	}
	/* case: it's the interval timer */
	if(lineNum == ITLINE)
	{
		ITInterruptHandler();
	}
	/* case: it's a device line */
	else
	{
		devregarea_t* busRegArea = (devregarea_t*) RAMBASEADDR;
		unsigned int devBitMap = busRegArea->interrupt_dev[lineNum - INITIALDEVLINENUM]; /* dev bit map for the line with an interrupt */
		int devNum;
		int transmitBool = TRUE; 									/* true if an interrupt on line 7 is transmit, false if receive */
		device_t* deviceAddr;										/* address for device with interrupt */
		int index;													/* index of the device's sema4 */
		int * Sema4;												/* device's sema4 */

		devNum = DetermineDevice(devBitMap);

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
					proc->p_s.s_v0 = deviceAddr->d_status;
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
				deviceAddr->d_command = ACK;
			}
		}
	}

	/* we're done here, so let's get another process up and running */
	ExitInterruptHandler(interruptOld);
}

HIDDEN int DetermineLine(unsigned int causeReg)
/* Examines the cause register from the old interrupt processor state area, and determines not only
which line has an interrupt, but which is of the highest priority. The cause register contains an 
Interrupts Pending (IP) field of 8 bits. The ith bit is a 1 if there is a pending interrupt on line i.
Returns the line number of the highest priority line with a pending interrupt. */
{
	int i = 0;														/* loop control variable for determining interrupt line number */
	unsigned int lineOn = LINE0;									/* indicates an interrupt on line i */
	unsigned int interruptOn;										/* all 0s if there is NOT an interrupt on line i */
	int foundLine = FALSE; 											/* true if the interrupting line has been found, false otherwise */
	int lineNum; 													/* highest priority line with interrupt */

	/* find highest priority line with interrupt */
	while((i < NUMLINES) && (!foundLine))
	{
		causeReg = causeReg & MASKCAUSEREG; 						/* zero out irrelevant bits */
		interruptOn = causeReg & lineOn; 							/* all 0s if there is NOT an interrupt on line i */

		/* case: line i has an interrupt */
		if(interruptOn != 0)
		{
			foundLine = TRUE;
			lineNum = i;
		}
		i++; 														/* check the next line */
		lineOn = lineOn << 1;
	}

	return lineNum;
}

HIDDEN int DetermineDevice(unsigned int devBitMap)
{
		int j = 0; 													/* loop control variable for determining interrupt device number */
		unsigned int deviceOn = DEVICE0;							/* indicates an interrupt on device i */
		unsigned int interruptOn;
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

	return devNum;
}

HIDDEN void PLTInterruptHandler()
{
	/* end the current process's turn */
	IncrementProcessTime(currentProcess);
	insertProcQ(&readyQ, currentProcess);
	Scheduler();													/* schedule a new process to run */
}

HIDDEN void ITInterruptHandler()
{
	int* sema4 = &(semaphoreArray[ITSEMINDEX]);						/* interval timer's semaddr */
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

HIDDEN void ExitInterruptHandler(state_PTR interruptOld)
{
	/* case: there was no process that got interrupted */
	if(currentProcess == NULL)
	{
		Scheduler();
	}																/* schedule a new process to run */
	/* case: a process got interrupted */
	else
	{
		LoadState(interruptOld); 									/* return control to the interrupted process*/
	}

}
