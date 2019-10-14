/************************************* PCB.C ********************************************************************
 *  written by Scott Harrington and Kara Schatz 								*
 *  														*
 * Purpose: from pcb.e,creates a  Queue Manager for a proccess queue and Free					*
 * Pcb list, and the proccess tree. 										*
 *														*
 *  Process Queue - circular, doubly linked list with a pointer to the tail of 					*
 *  of PCBs that are in use. FIFO										*
 * 														*
 *  Free List- circular, doubly linnked list with a pointer to the tail of free					*
 *  PCBS. FIFO													*
 * 														*
 *  Proccess Tree- each parent points to the child, where the child points to the siblings, which are		*
 *   a null terminated stack.											*
 *  														*
 *  														*
 *														*
 * includes const, types, pcb.e, 										*
 ****************************************************************************************************************/
#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"





/******************************  Process Queue *****************************************************************/
HIDDEN pcb_PTR pcbList_h;

/* manage "free" list of PCBs */
void freePcb(pcb_PTR p)
/* Insert the element pointed to by p onto the pcbFree list. */
{
	insertProcQ(&pcbList_h, p);
}


pcb_PTR allocPcb()
/* Return NULL if the pcbFree list is empty. Otherwise, remove
an element from the pcbFree list, provide initial values for ALL
of the ProcBlk’s fields (i.e. NULL and/or 0) and then return a
pointer to the removed element. ProcBlk’s get reused, so it is
important that no previous value persist in a ProcBlk when it
gets reallocated. */
{
	pcb_PTR elem;
	int i;
	/* empty list case */
	if(emptyProcQ(pcbList_h)){
		return(NULL);
	}
	/* non-empty list case */
	else{
		elem = removeProcQ(&pcbList_h);
		/* initialize/clear out all fields */
		elem->p_prev = NULL;
		elem->p_next = NULL;
		elem->p_prnt = NULL;
		elem->p_child = NULL;
		elem->p_sib_prev = NULL;
		elem->p_sib_next = NULL;
		elem->p_semAdd = NULL;
		elem->p_totalTime = 0;
		for(i = 0; i < NUMEXCEPTIONTYPES; i++){
			elem->oldAreas[i] = NULL;
			elem->newAreas[i] = NULL;
		}
	}
	return(elem);
}

void initPcbs()
/* Initialize the pcbFree list to contain all the elements of the
static array of MAXPROC ProcBlk’s. This method will be called
only once during data structure initialization.
 */
{
	int i;
	static pcb_t procTable[MAXPROC];

	pcbList_h = mkEmptyProcQ();

	for(i=0; i < MAXPROC; i++){
		freePcb(&(procTable[i]));
	}
}

/* queue management */

/* return type = pointer to pcb */
pcb_PTR mkEmptyProcQ()
/* This method is used to initialize a variable to be tail pointer to a
process queue.
Return a pointer to the tail of an empty process queue; i.e. NULL. */
{
	return(NULL);
}

int emptyProcQ(pcb_PTR tp)
/* Return TRUE if the queue whose tail is pointed to by tp is empty.
Return FALSE otherwise. */
{
	return(tp == NULL);
}

void insertProcQ(pcb_PTR *tp, pcb_PTR p)
/* Insert the ProcBlk pointed to by p into the process queue whose
tail-pointer is pointed to by tp. Note the double indirection through
tp to allow for the possible updating of the tail pointer as well. */
{
  /* empty queue case */
	if(emptyProcQ(*tp)){
		*tp = p; 			/* make p the only  node */
		p->p_prev = p;
		p->p_next = p;
	}
    /* non-empty queue case */
    else {
		p->p_next = (*tp)->p_next; 	/* assign p's next to be current tail's next */
		(*tp)->p_next->p_prev = p;	/* assign head's prev to be p */
		p->p_prev = *tp; 		/* assign p's previous to be current tail */
		(*tp)->p_next = p; 		/* assign current tail's next to be p */
		*tp = p;
    }
}

pcb_PTR removeProcQ(pcb_PTR *tp)
/* Remove the first (i.e. head) element from the process queue whose
tail-pointer is pointed to by tp. Return NULL if the process queue
was initially empty; otherwise return the pointer to the removed element. Update the process queue’s tail pointer if necessary. */
{
	/* empty queue case */
	if(emptyProcQ(*tp)){
		return(NULL);
	}
	/* single-element queue case */
	else if(*tp == headProcQ(*tp)){
	    pcb_PTR head = *tp;
	    *tp = mkEmptyProcQ(); 			/* was last element, so it makes the list empty */
	    return(head);
	}
	/* multi-element queue case */
	else{
		pcb_PTR head = (*tp)->p_next;
		(*tp)->p_next = (*tp)->p_next->p_next; 	/* assign tail's next to be new head (what was the second elem) */
		(*tp)->p_next->p_prev=*tp; 		/* assign new head's prev to the tail*/
		return(head);
	}
}

pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p)
/* Remove the ProcBlk pointed to by p from the process queue whose
tail-pointer is pointed to by tp. Update the process queue’s tail
pointer if necessary. If the desired entry is not in the indicated queue
(an error condition), return NULL; otherwise, return p. Note that p
can point to any element of the process queue. */
{
	/* empty queue case */
	if(emptyProcQ(*tp)){
		return(NULL);
	}
	/* single-element queue case */
	else if(*tp == headProcQ(*tp)){
	    /* tail is the node to remove case */
	    if(*tp == p){
	    	return(removeProcQ(tp));
	    }
	    /* p DNE case*/
	    else{
		return(NULL);
	    } 
	}
	/* multi-element queue case */
	else{
		/* tail is the node to remove case */ 
	  	if(*tp == p){
	    		*tp = (*tp)->p_prev;
	    		return(removeProcQ(tp));
		}
		/* remove some internal node case */
	  	else{
	  		int at_tail = FALSE; 					/* tracker to see if looped all the way back around to tail, i.e. p DNE */
			pcb_PTR current_elem = *tp;
			while(!at_tail){					/* loopy loop- searching for pcb p within the queue */
				if(current_elem == p){
				  pcb_PTR temp = current_elem->p_prev;		/* we found it in the loop*/
				  return(removeProcQ(&temp));			
				}
				else{
					current_elem = current_elem->p_next;	/* set the elem to its next, to contiune looking for p */
					if(current_elem == *tp){
						at_tail = TRUE;			
					}
					else{					
						at_tail = FALSE;			
					}
				}
			}
			/* looped through entire queue, so p DNE  case*/
			return(NULL);
	  	}
	}
}

pcb_PTR headProcQ(pcb_PTR tp)
/* Return a pointer to the first ProcBlk from the process queue whose
tail is pointed to by tp. Do not remove this ProcBlkfrom the process
queue. Return NULL if the process queue is empty. */
{
	/* empty queue case */
	if(emptyProcQ(tp)){
		return(NULL);
	}
	/* non-empty queue case */
	else{
		return(tp->p_next);
	}
}

 /* end of  process queue*/



/***************************************** Process Tree  *************************************************************/

int emptyChild(pcb_PTR p)
/* Return TRUE if the ProcBlk pointed to by p has no children. Return FALSE otherwise. */
{
	/* children DNE */
	if(p->p_child == NULL){
		return TRUE;
	}
	/* children exist */
	else{
		return FALSE;
	}
}

void insertChild(pcb_PTR prnt, pcb_PTR p)
/* Make the ProcBlk pointed to by p a child of the ProcBlk pointed
to by prnt. */
{
	p->p_prnt = prnt; 				
	/* first child case */
	if(emptyChild(prnt)){
		prnt->p_child = p;
	}
	
	else { /* not first child case */
		p->p_sib_next = prnt->p_child; 		/* assign p's next sib to be prnt's child */
		prnt->p_child->p_sib_prev = p;		/* assign the prnt's child's prev sib to be p */
		prnt->p_child = p;			/* assign prnt's child to be p*/
	}
}

pcb_PTR removeChild(pcb_PTR p)
/* Make the first child of the ProcBlk pointed to by p no longer a
child of p. Return NULL if initially there were no children of p.
Otherwise, return a pointer to this removed first child ProcBlk. */
{
	if(emptyChild(p)){
		return(NULL);
	}
	else if(p->p_child->p_sib_next == NULL && p->p_child->p_sib_prev == NULL){
		pcb_PTR child = p->p_child;
		p->p_child = NULL;
		return(child);
	}
	else {
	  	pcb_PTR child = p->p_child;
		/*make p's sib the parent's child child*/
		p->p_child = child->p_sib_next;
		p->p_child->p_sib_prev = NULL;

		/*change child's prnt and sib??*/
		return(child);
	}
}

pcb_PTR outChild(pcb_PTR p)
/* Make the ProcBlk pointed to by p no longer the child of its parent.
If the ProcBlk pointed to by p has no parent, return NULL; otherwise,
return p. Note that the element pointed to by p need not be the first
child of its parent. */
{
	/* parent DNE */
	if(p->p_prnt == NULL){
		return(NULL);
	}
	/* parent exists */
	else{
		pcb_PTR parent = p->p_prnt;
		/* want to remove 1st child case*/
		if(parent->p_child == p){
			return(removeChild(parent));
		}
		/* want to remove internal child case*/
		else{
			int done = FALSE;
			pcb_PTR current_child = parent->p_child;
			while(!done){
				if(current_child == p){
					/* remove case */
					if(p->p_sib_next == NULL){
						p->p_sib_prev->p_sib_next = p->p_sib_next;  /* setting p previous next to be p's next, in which case is null, so we are done after that*/
					}
					else {
						p->p_sib_prev->p_sib_next = p->p_sib_next;  /* setting p previous next to be p's next, */
						p->p_sib_next->p_sib_prev = p->p_sib_prev;  /* setting p next previous to be p's  previous, getting rid of the middling node */
					}
					done = TRUE; 
				}
				else{
					current_child = current_child->p_sib_next;	    /*if we we didnt find it, continue looping  */
				}
			}
			return(p);
		}
	}
} 
