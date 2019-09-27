#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/

/* Hardware & software constants */
#define PAGESIZE		4096	/* page size in bytes */
#define WORDLEN			4		/* word size in bytes */
#define PTEMAGICNO		0x2A


#define ROMPAGESTART	0x20000000	 /* ROM Reserved Page */


/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR		0x10000000
#define TODLOADDR		0x1000001C /* C or c? */
#define INTERVALTMR		0x10000020	
#define TIMESCALEADDR	0x10000024

/* old and new processor state areas */
#define SYSCALLNEWAREA			0x200003D4
#define SYSCALLOLDAREA			0x20000348
#define PROGRAMTRAPNEWAREA		0x200002BC
#define PROGRAMTRAPOLDAREA		0x20000230
#define TLBMANAGEMENTNEWAREA	0x200001A4
#define TLBMANAGEMENTOLDAREA	0x20000118
#define INTERRUPTNEWAREA		0x2000008C
#define INTERRUPTOLDAREA		0x20000000

/* values for controlling bits in the status register */
#define ALLOFF					0x00000000
#define VMON					0x01000000
#define VMOFF					0x00000000
#define KERNELON				0x00000000
#define KERNELOFF				0x00000008
#define INTERRUPTSMASKED 		0x00000000
#define INTERRUPTSUNMASKED		0x00000004
#define CURRINTERRUPTSUNMASKED	0x00000001
#define INTERRUPTMASKOFF 		0x00000000 	/* change this later maybe */
#define INTERRUPTMASKON			0x0000FF00

/* need one semaphore for each external device that is not terminal + 2 for each terminal device + 1 for the clock
	external devices:
		8 disk devices
		8 tape devices
		8 network adapters
		8 printer devices
		8 terminal devices
	so 4*8 + 2*8 + 1 = 49 total semaphores 
	The order of the semaphore array is as follows:	
		[timer, 8 disk devices, 8 tape devices, 8 network adapters, 8 printer devices, 8 terminal devices] */
#define SEMCOUNT 	49

/* syscall services */
#define CREATEPROCESS 		1
#define TERMINATEPROCESS	2
#define VERHOGEN			3
#define PASSEREN			4
#define EXCEPTIONSTATEVEC	5
#define	GETCPUTIME			6
#define WAITFORCLOCK		7
#define WAITFORIO			8

/* exception types */
#define TLBEXCEPTION			0
#define PROGRAMTRAPEXCEPTION	1
#define SYSCALLEXCEPTION		2
#define NUMEXCEPTIONTYPES		3

/* utility constants */
#define	TRUE			1
#define	FALSE			0
#define ON              1
#define OFF             0
#define HIDDEN			static
#define EOS				'\0'

#define NULL ((void *)0xFFFFFFFF)


/* vectors number and type */
#define VECTSNUM	4

#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2

#define TRAPTYPES	3


/* device interrupts */
#define DISKINT		3
#define TAPEINT 	4
#define NETWINT 	5
#define PRNTINT 	6
#define TERMINT		7

#define DEVREGLEN	4	/* device register field length in bytes & regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS		0
#define COMMAND		1
#define DATA0		2
#define DATA1		3

/* device register field number for terminal devices */
#define RECVSTATUS      0
#define RECVCOMMAND     1
#define TRANSTATUS      2
#define TRANCOMMAND     3


/* device common STATUS codes */
#define UNINSTALLED	0
#define READY		1
#define BUSY		3

/* device common COMMAND codes */
#define RESET		0
#define ACK			1

/* operations */
#define	MIN(A,B)	((A) < (B) ? A : B)
#define MAX(A,B)	((A) < (B) ? B : A)
#define	ALIGNED(A)	(((unsigned)A & 0x3) == 0)

/* Useful operations */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 

/* pcb values */
#define MAXPROC 	20

#endif
