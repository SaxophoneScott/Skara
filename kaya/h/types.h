#ifndef TYPES
#define TYPES

/****************************************************************************
 *
 * This header file contains utility types definitions.
 *
 ****************************************************************************/


typedef signed int cpu_t;


typedef unsigned int memaddr;


typedef struct {
	unsigned int d_status;
	unsigned int d_command;
	unsigned int d_data0;
	unsigned int d_data1;
} device_t;

#define t_recv_status		d_status
#define t_recv_command		d_command
#define t_transm_status		d_data0
#define t_transm_command	d_data1

#define DEVINTNUM 5
#define DEVPERINT 8
typedef struct {
	unsigned int rambase;
	unsigned int ramsize;
	unsigned int execbase;
	unsigned int execsize;
	unsigned int bootbase;
	unsigned int bootsize;
	unsigned int todhi;
	unsigned int todlo;
	unsigned int intervaltimer;
	unsigned int timescale;
	unsigned int inst_dev[DEVINTNUM];
	unsigned int interrupt_dev[DEVINTNUM];
	device_t   devreg[DEVINTNUM * DEVPERINT];
} devregarea_t;

#define STATEREGNUM	31
typedef struct state_t {
	unsigned int	s_asid;
	unsigned int	s_cause;
	unsigned int	s_status;
	unsigned int 	s_pc;
	int	 			s_reg[STATEREGNUM];

} state_t, *state_PTR;

#define	s_at	s_reg[0]
#define	s_v0	s_reg[1]
#define s_v1	s_reg[2]
#define s_a0	s_reg[3]
#define s_a1	s_reg[4]
#define s_a2	s_reg[5]
#define s_a3	s_reg[6]
#define s_t0	s_reg[7]
#define s_t1	s_reg[8]
#define s_t2	s_reg[9]
#define s_t3	s_reg[10]
#define s_t4	s_reg[11]
#define s_t5	s_reg[12]
#define s_t6	s_reg[13]
#define s_t7	s_reg[14]
#define s_s0	s_reg[15]
#define s_s1	s_reg[16]
#define s_s2	s_reg[17]
#define s_s3	s_reg[18]
#define s_s4	s_reg[19]
#define s_s5	s_reg[20]
#define s_s6	s_reg[21]
#define s_s7	s_reg[22]
#define s_t8	s_reg[23]
#define s_t9	s_reg[24]
#define s_gp	s_reg[25]
#define s_sp	s_reg[26]
#define s_fp	s_reg[27]
#define s_ra	s_reg[28]
#define s_HI	s_reg[29]
#define s_LO	s_reg[30]

#define TRAPTYPES 3
/* process control block type */
typedef struct pcb_t{
	/* process queue fields */
	struct pcb_t	*p_next, 	/* pointer to next entry */
					*p_prev,	/* pointer to prev entry */
	/* process tree fields */
					*p_prnt, 	/* pointer to parent */
					*p_child, 	/* pointer to 1st child */
					*p_sib_prev, 	/* pointer to prev sibling */
					*p_sib_next;	/* pointer to next sibling */

	state_t		p_s;		/* processor state */
	int 		*p_semAdd;	/* pointer to sema4 on which process is blocked */

	/* cpu_t 		p_startTime; */
	cpu_t		p_totalTime;

	state_t*	oldAreas[TRAPTYPES];	/* [TLB, programtrap, syscall] */
	state_t*	newAreas[TRAPTYPES];	/* [TLB, programtrap, syscall] */

} pcb_t, *pcb_PTR;

/* semaphore descriptor type */
typedef struct semd_t{
	struct semd_t 	*s_next; 	/* next element on the ASL */
	int 			*s_semAdd;	/* pointer to the semaphore */
	pcb_t 			*s_procQ; 	/* tail pointer to a process queue */
} semd_t;

/* page table entry */
typedef struct pagetbe_t{
	unsigned int 	entryHi;
	unsigned int 	entryLo;
} pagetbe_t;

/* kuseg2/3 page table */
typedef struct kupagetable_t{
	unsigned int 	header;
	struct pagetbe_t 		entries[MAXKUSEG];
} kupagetable_t;

/* ksegos3 page table */
typedef struct ospagetable_t{
	struct pagetbe_t 		entries[MAXKSEGOS];
	unsigned int 		header;
} ospagetable_t;

/* segment table */
typedef struct segtable_t{
	ospagetable_t*			ksegos;
	kupagetable_t*			kuseg2;
	kupagetable_t*			kuseg3;
} segtable_t;

/* swap pool entry */
typedef struct frameswappoole_t{
	int 			ASID,
				segNum,
				pageNum;
	pagetbe_t*		pagetbentry; 	/* optional */
} frameswappoole_t;

typedef struct upcb_t{
	int* 			sema4;
	kupagetable_t*		kuseg2PT;
	unsigned int 		backingStoreAddr;
	state_t*		oldAreas[TRAPTYPES];	/* [TLB, programtrap, syscall] */
	state_t*		newAreas[TRAPTYPES];	/* [TLB, programtrap, syscall] */

} upcb_t;

#endif
