
#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"

HIDDEN pcb_PTR pcbList_h;

/* manage "free" list of PCBs */
void freePcb(pcb_t *p)
/* Insert the element pointed to by p onto the pcbFree list. */
{
	insertProcQ(&pcbList_h, &p);
}


pcb_PTR allocPcb()
/* Return NULL if the pcbFree list is empty. Otherwise, remove
an element from the pcbFree list, provide initial values for ALL
of the ProcBlk’s fields (i.e. NULL and/or 0) and then return a
pointer to the removed element. ProcBlk’s get reused, so it is
important that no previous value persist in a ProcBlk when it
gets reallocated. */
{
	if(pcbList_h == NULL){
		return(NULL);
	}
	else{
		return(pcbList_h.dequeue());
	}
}

void initPcbs()
/* Initialize the pcbFree list to contain all the elements of the
static array of MAXPROC ProcBlk’s. This method will be called
only once during data structure initialization. */
{
	static pcb_t procTable[MAXPROC];

	pcbList_h = QUEUE;

	for(i=0; i < MAXPROC; i++){
		freePcb(&(procTable[i]));
	}
}

/* queue management */

/* return type = pointer to pcb */
pcb_t* mkEmtpyProcQ()
/* This method is used to initialize a variable to be tail pointer to a
process queue.
Return a pointer to the tail of an empty process queue; i.e. NULL. */
{
	return(NULL);
}

int emptyProcQ(pcb_t* tp)
/* Return TRUE if the queue whose tail is pointed to by tp is empty.
Return FALSE otherwise. */
{
	return(tp == NULL);
}

void insertProcQ(pcb_t **tp, pcb_t *p)
/* Insert the ProcBlk pointed to by p into the process queue whose
tail-pointer is pointed to by tp. Note the double indirection through
tp to allow for the possible updating of the tail pointer as well. */
{
  /* empty queue case */
	if(emptyProcQ(*tp)){
		/* set tail to new node */
		*tp = p;
		/* assign p_next to point to itself */
		p->p_next = p;
		/* assign p_prev to point to itself */
		p->p_prev = p;
	} 
    /* non-empty queue case */
    else {
		/* assign the current tail's next to p's next */
		p->p_next = (*tp)->p_next;
		/* assign the heads previous to p */
		(*tp)->p_next->p_prev = p;
		/* assign p's previous to tail */
		p->p_prev = *tp;
		/* assign the current tail's next to be p */
		(*tp)->p_next = p;
		/* change tail to p */
		*tp = p;
    }
}

pcb_t *removeProcQ(pcb_t **tp)
/* Remove the first (i.e. head) element from the process queue whose
tail-pointer is pointed to by tp. Return NULL if the process queue
was initially empty; otherwise return the pointer to the removed element. Update the process queue’s tail pointer if necessary. */
{
	/* empty queue case */
	if(emptyProcQ(*tp)){
		return(NULL);
	}
	/* if the tail pointer is also the head, set **tp to null, and remove it from the list*/
	else if(*tp == headProcQ(*tp)){
	    pcb_PTR head = *tp;
	    *tp = (NULL);
	    return(head);
	}
	else{
		pcb_PTR head = tp->p_next;
		/*assigns the new heads previous to the tail*/
		head->p_next->p_prev=*tp;
		/*assigns the tail's next to be it's next's next*/
		(*tp)->p_next = (*tp)->p_next->p_next;
		return(head);
	}
}

pcb_t *outProcQ(pcb_t **tp, pcb_t *p)
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
	/* length = 1 */
	else if(*tp == headProcQ(*tp)){
	    /* check to see if the tail pointer is the same as the proccess block*/
	    if(*tp == p){
	    	removeProcQ(tp);
	    }
	    /* otherwise, its doesnt exist, so :(*/
	    else{
			return(NULL);
	    } 
	}
	/* length >= 2 */
	else{
		/* remove tail */ 
	  	if(*tp == p){
	    	*tp = *tp->p_prev;
	    	removeProcQ(tp);
		}
	  	else{
	  		at_tail = 0;
			pcb_PTR current_elem = *tp;
			while(!at_tail){
				if(current_elem == p){
				  pcb_PTR temp = current_elem->p_prev;
				  removeProcQ(&temp);
				}
				else{
					current_elem = current_elem->p_next;
					if(current_elem == *tp){
						at_tail = 1;
					}
					else{
						at_tail = 0;
					}
				}
			}
			return(NULL);
	  	}
	}
}

pcb_t *headProcQ(pcb_t *tp)
/* Return a pointer to the first ProcBlk from the process queue whose
tail is pointed to by tp. Do not remove this ProcBlkfrom the process
queue. Return NULL if the process queue is empty. */
{
	if(emptyProcQ(*tp)){
		return(NULL);
	}
	else{
		return(tp->p_next);
	}
}

/* parent/child manager */

int emptyChild(pcb_t *p)
/* Return TRUE if the ProcBlk pointed to by p has no children. Return FALSE otherwise. */
{
	if(p->p_child == NULL){
		return 1;
	}
	else{
		return 0;
	}
}

void insertChild(pcb_t *prnt, pcb_t *p)
/* Make the ProcBlk pointed to by p a child of the ProcBlk pointed
to by prnt. */
{
	/*make p's parent prnt*/
	p->p_prnt = prnt;
	/*make the prnt's child p's next sib*/
	p->p_sib_next = prnt->p_child;
	/* make the prnt's child's prev sib p */
	prnt->p_child->p_sib_prev = p;
	/*make p the prnt's child*/
	prnt->p_child = p;

}

pcb_t *removeChild(pcb_t *p)
/* Make the first child of the ProcBlk pointed to by p no longer a
child of p. Return NULL if initially there were no children of p.
Otherwise, return a pointer to this removed first child ProcBlk. */
{
	if(emptyChild(p)){
		return(NULL);
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

pcb_t *outChild(pcb_t *p)
/* Make the ProcBlk pointed to by p no longer the child of its parent.
If the ProcBlk pointed to by p has no parent, return NULL; otherwise,
return p. Note that the element pointed to by p need not be the first
child of its parent. */
{
	/* it has no parent */
	if(p->p_prnt == NULL){
		return(NULL);
	}
	/* it has a parent */
	else{
		pcb_PTR parent = p->p_prnt;
		/* it's the first child */
		if(parent->p_child == p){
			removeChild(parent);
		}
		/* it's not the first child */
		else{
			done = 0
			current_child = parent->p_child;
			while(!done){
				if(current_child == p){
					/* remove */
					current_child->p_sib_prev->p_sib_next = current_child->p_sib_next;
					current_child->p_sib_next->p_sib_prev = current_child->p_sib_prev;
					done = 1;
					return(p);
				}
				else{
					current_child = current_child->p_sib_next;
				}
			}
		}
	}
}
                  
