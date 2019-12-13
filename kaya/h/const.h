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
#define SYSCALLNEWAREA				0x200003D4
#define SYSCALLOLDAREA				0x20000348
#define PROGRAMTRAPNEWAREA			0x200002BC
#define PROGRAMTRAPOLDAREA			0x20000230
#define TLBMANAGEMENTNEWAREA		0x200001A4
#define TLBMANAGEMENTOLDAREA		0x20000118
#define INTERRUPTNEWAREA			0x2000008C
#define INTERRUPTOLDAREA			0x20000000

/* values for controlling bits in the status register */
#define ALLON						0xFFFFFFFF
#define ALLOFF						0x00000000
#define VMONCURR					0x01000000
#define VMONPREV					0x02000000 /* 0x02000000 */
#define INITVMOFF					0x00000000
#define KERNELON					0x00000000
#define KERNELOFF					0x00000008
#define INTERRUPTSMASKED 			0x00000000
#define INTERRUPTSUNMASKED			0x00000004
#define CURRINTERRUPTSUNMASKED		0x00000001
#define INTERRUPTMASKOFF 			0x00000000 	/* change this later maybe */
#define INTERRUPTMASKON				0x0000FF00
#define TEBITON						0x08000000	/* timer enable bit*/

/* values for controlling bits in the cause register */
#define PRIVILEDGEDINSTR		0x00000028
#define EXCCODEMASK				0x0000003C
#define EXCCODESHIFT			2
#define TLBINVALIDLOAD			2
#define TLBINVALIDSTORE			3

/* values for managing the array of device semaphores and computing device register locations */
/* need one semaphore for each external device that is not terminal + 2 for each terminal device + 1 for the clock
	external devices:
		8 disk devices
		8 tape devices
		8 network adapters
		8 printer devices
		8 terminal devices
	so 4*8 + 2*8 + 1 = 49 total semaphores
	The order of the semaphore array is as follows:
		[8 disk devices, 8 tape devices, 8 network adapters, 8 printer devices, 8 terminal device transmitters, 8 terminal device receivers, timer] */
#define DEVICECOUNT				48
#define SEMCOUNT 				DEVICECOUNT + 1
#define ITSEMINDEX				48
#define INITIALDEVLINENUM 		3
#define NUMDEVICESPERTYPE 		8
#define BASEDEVICEADDRESS		0x10000050
#define DEVICETYPESIZE 			0x00000080
#define DEVICESIZE				0x00000010

/* syscall services */
#define CREATEPROCESS 		1
#define TERMINATEPROCESS	2
#define VERHOGEN			3
#define PASSEREN			4
#define EXCEPTIONSTATEVEC	5
#define	GETCPUTIME			6
#define WAITFORCLOCK		7
#define WAITFORIO			8
/* other important syscall values */
#define MINSYSCALL			1
#define MAXSYSCALL			8
#define SYS1FAIL			-1
#define SYS1SUCCESS			0

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

/* possible trap types */
#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2
#define TRAPTYPES	3

/* values for determining line number in interrupts pending IP bits of cause reg
and device number in interruping devices bit map */
#define MASKCAUSEREG		0x0000FF00
#define NUMLINES			8
#define LINE0				0x00000100
#define PLTLINE				1
#define ITLINE 				2
/* #define INTERRUPTLINES[NUMLINES]	{0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000} */
#define MASKDEVBITMAP		0x000000FF
#define NUMDEVICES 			8
#define DEVICE0				0x00000001
/* #define INTERRUPTDEVICES[NUMDEVICES]	 {0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080} */
#define MASKTRANSCHAR		0x0000000F

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

/* timing constants */
#define PLTTIME 		5000
#define INTERVALTIME	100000
#define SUSPENDTIME 	420420

/* phase 3 things */

/* segment ids */
#define KUSEG2 				2
#define KUSEG3 				3

/* values for structure sizes */
#define PROCCNT				1
#define MAXKUSEG			32
#define MAXKSEGOS			80 /* MAXKUSEG * 2 */
#define POOLSIZE			PROCCNT * 2  /* change this */
#define OSFRAMES			30
#define UPROCSTACKSIZE		3

/* memory addresses for segments and RAM organization */
#define SEGMENTTABLE		0x20000500
#define KSEGOSSTART			0x20000
#define KUSEG2START			0x80000
#define	KUSEG3START			0xC0000
#define KUSEG2LAST			0xBFFFF
#define LASTPAGEKUSEG2		0xC0000000
#define UPROCPCINIT			0x80000004 /* 0x800000B0 */
#define LEGALADDRSTART		0x80000000
/* buffer and stack stuff */
#define TAPEBUFFERSTART		ROMPAGESTART + (OSFRAMES * PAGESIZE)
#define DISKBUFFERSTART		TAPEBUFFERSTART + (NUMDEVICES * PAGESIZE)
#define STACKPOOLSTART		DISKBUFFERSTART + (NUMDEVICES * PAGESIZE)

/* values for managing page tables and frame swap pool */
#define MAGICNUM			0x2A
#define MAGICNUMSHIFT		24
#define SEGMASK				0xC0000000
#define SEGSHIFT			30
#define PAGEMASK			0x3FFFF000
#define PAGESHIFT			12
#define ASIDMASK			0x00000FC0
#define ASIDSHIFT			6
#define FRAMENUMMASK		0x00000FFF
#define FRAMESHIFT			12
#define DIRTYON				0x00000400
#define VALIDON				0x00000200 /* 0x00000100 */
#define GLOBALON			0x00000100 /* 0x00000080 */
#define VALIDMASK			0x00000200
#define VALIDSHIFT			9

#define UNOCCUPIEDFRAME		-1

/* semaphore initialization values */
#define MUTEXINIT			1
#define SYNCINIT			0

/* values to modify interrupt permissions */
#define ENABLEINTERRUPTS	0x00000001 /* 0x00000004 */
#define DISABLEINTERRUPTS	0xFFFFFFFE /* 0xFFFFFFFB */

/* values for devices */ 
#define BACKINGSTORE		0
#define DISKLINE			3
#define TAPELINE			4
#define PRINTERLINE			6
#define TERMINALLINE		7

/* values to manage device registers */
#define DEVICECOMMANDSHIFT	8
#define READBLK				3
#define SEEKCYL				2
#define WRITEBLK			4
#define TRANSMITCHAR		2
#define TERMINALSTATUSMASK	0xFF
#define EOT			0
#define EOF			1
#define EOB					2
#define CHARTRANSMITTED		5

/* values for determine disk heads */
#define KUSEG2HEAD			0
#define KUSEG3HEAD			1

/* user level syscalls */
#define READFROMTERMINAL	9
#define WRITETOTERMINAL		10
#define VVIRTUAL			11
#define PVIRTUAL			12
#define DELAY				13
#define DISKPUT				14
#define DISKGET				15
#define WRITETOPRINTER		16
#define GETTOD				17
#define USERTERMINATE 		18

#endif
