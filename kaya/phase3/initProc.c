
/* phase 3 global variables */

void test() 
{
	int i;

	/* initializing all phase 3 globals */
	/* ksegOS page table */
	for(i = 0; i < 64; i++)
	{
		/* entryHi = 0x20000 + i (ASID is irrelephant) */
		/* entryLo = 0x20000 + i with dirty, valid, global */
	}

	/* init kuseg3 page table */
	for(i = 0; i < 32; i++)
	{
		/* entryHi = 0xC0000 + i */
		/* entryLo = dirty, global */
	}

	/* swap pool */
	for(each entry)
	{
		/* ASID = -1 to indicate unoccupied */
	}

	/* swap pool sema4 = 1 (mutex) */

	/* device sema4s */
	for(i = 0, i < 48; i++)
	{
		/* = 1 (mutex) */
	}

	/* master sema4 = 0 (to know when all processes are done and it should SYS2 itself) */


	for(i = 1; i < PROCCNT+ 1; i++)
	{
		/* setup proc i's  kuseg2 page table */
		for(i = 0; i < 32; i++)
		{
			/* entryHi = 0x80000 + i with ASID i */
			/* entryLo = no frame #, dirty, not valid, not global */
		}
		/* fix last entry's entryHi to be oxBFFFF w/ ASID i */

		/* setup appropriate 3 entries (for proc i) in global segment table */
			/* ksegOS = global var table
			   kuseg2 = process's table we just set up
			   kuseg3 = global var table
			*/

		/* setup initial process state */
			/* ASID = i
			   stack page = TBD
			   pc = uProcInit method
			   status = all interrupts enabled, PLT enabled, VM off, kernel mode on
			*/

		/* sys 1 */
	}
}

void processSetup()
{
	/* init puseg2 page table 
		3 sys 5s
		read code from tape to backing store
		LDST */
}