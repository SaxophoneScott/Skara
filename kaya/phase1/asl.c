/****************************** ASL.c ******************************
* written by Scott Harrington and Kara Schatz                      *
* 																   *
* Purpose: Implements both an active and free semaphore list, and  *
* contains functions necessary to manage both these lists.         *
*                                                                  *
* Active Semaphore List (ASL): sorted NULL-terminated single       *
* linearly linked list, uses a dummy node on each end of the list  *
* for algorithmis ease, sorted in ascending order using the        *
* s_semAdd field as the sort key                                   *
*                                                                  *
* Free Semaphore List: standard stack                              *
*                                                                  *
*******************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../e/asl.e"
#include "../e/pcb.e"

HIDDEN semd_t *semd_h, *semdFree_h;
/* prototypes for local funcs */
HIDDEN semd_t* allocSemd();
HIDDEN void freeSemd(semd_t* s);
HIDDEN semd_t* searchASL(int* semAdd);

void initASL()
/* Initialize the semdFree list to contain all the elements of the array
static semd t semdTable[MAXPROC]
This method will be only called once during data structure initialization.
*/
{
	int i;
	semd_t* dummy1;
	semd_t* dummy2;
	static semd_t semdTable[MAXPROC+2];

	semdFree_h = NULL;

	/* loop to add all semd_ts to the free semaphore list */
	for(i=0; i < MAXPROC+2; i++){
		freeSemd(&(semdTable[i]));
	}

	/* allocate two nodes to be dummy nodes for each end of the ASL */
	dummy1 = allocSemd();
	dummy2 = allocSemd();
	dummy1->s_semAdd = 0; 		/* front dummy should be 0 */
	dummy2->s_semAdd = NULL; 	/* end dummy should be MAX INT */
	semd_h = dummy1;
	dummy1->s_next = dummy2;
}

int insertBlocked(int *semAdd, pcb_PTR p)
/* Insert the ProcBlk pointed to by p at the tail of the process queue
associated with the semaphore whose physical address is semAdd
and set the semaphore address of p to semAdd. If the semaphore is
currently not active (i.e. there is no descriptor for it in the ASL), allocate
a new descriptor from the semdFree list, insert it in the ASL (at
the appropriate position), initialize all of the fields (i.e. set s semAdd
to semAdd, and s procq to mkEmptyProcQ()), and proceed as
above. If a new semaphore descriptor needs to be allocated and the
semdFree list is empty, return TRUE. In all other cases return FALSE.
*/
{
	semd_t* temp = searchASL(semAdd);
	/* semAdd is in ASL case */
	if(temp->s_next->s_semAdd == semAdd){
		/* insert p into the procQ */
  		insertProcQ(&(temp->s_next->s_procQ),p);
  		p->p_semAdd = semAdd;
  		return(FALSE);
  	}
	/* semAdd is not in ASL case */
  	else{
		semd_t* temp2 = allocSemd();
		/* error: no semds to allocate case */
		if(temp2 == NULL){
			return(TRUE);
  		}
		/* semd was allocated without issue case */
		else{
			/* initialize semd fields appropriately */
 			temp2->s_semAdd = semAdd;
  			temp2->s_procQ = mkEmptyProcQ();
  			temp2->s_next = temp->s_next;
  			temp->s_next = temp2;				/* add semd to ASL */
  			/* insert p into the procQ */
  			insertProcQ(&(temp2->s_procQ), p);
  			p->p_semAdd = semAdd;
  			return(FALSE);
		}
  	}
}

pcb_PTR removeBlocked(int *semAdd)
/* Search the ASL for a descriptor of this semaphore. If none is
found, return NULL; otherwise, remove the first (i.e. head) ProcBlk
from the process queue of the found semaphore descriptor and return
a pointer to it. If the process queue for this semaphore becomes
empty (emptyProcQ(s procq) is TRUE), remove the semaphore
descriptor from the ASL and return it to the semdFree list. */
{
  	semd_t* temp = searchASL(semAdd);
	/* semAdd is in ASL case */
  	if(temp->s_next->s_semAdd == semAdd){
  		pcb_PTR temp2 = removeProcQ(&(temp->s_next->s_procQ));
		/* the semd's procQ is now empty case */
  		if(emptyProcQ(&(*temp->s_next->s_procQ))){
  			/* take semd off ASL and add to free list */
			semd_t* temp3 = temp->s_next;
			temp->s_next = temp->s_next->s_next;
			freeSemd(temp3);
  		}
 		return(temp2);
    }
	/* semAdd is not in ASL case */
  	else{
  		return(NULL);
    }
}

pcb_PTR outBlocked(pcb_PTR p)
/* Remove the ProcBlk pointed to by p from the process queue associated
with p’s semaphore (p" p semAdd) on the ASL. If ProcBlk
pointed to by p does not appear in the process queue associated with
p’s semaphore, which is an error condition, return NULL; otherwise,
return p. */
{
  	semd_t* temp = searchASL(p->p_semAdd);
  	/* semAdd is in ASL case */ 
	if(temp->s_next->s_semAdd == p->p_semAdd){
  		pcb_PTR temp2 = outProcQ(&(temp->s_next->s_procQ), p);
  		/* semd's procQ is not empty case */
		if(emptyProcQ(temp->s_next->s_procQ)){
			/* take semd off ASL and add to free list */
	  		semd_t* temp3 = temp->s_next;
	  		temp->s_next = temp->s_next->s_next;
	  		freeSemd(temp3);
		}
  		return(temp2);
    }
	/* semAdd is not in ASL case */
  	else{
    	return(NULL);
	}
}

pcb_PTR headBlocked(int *semAdd)
/* Return a pointer to the ProcBlk that is at the head of the process
queue associated with the semaphore semAdd. Return NULL
if semAdd is not found on the ASL or if the process queue associated
with semAdd is empty. */
{
  	semd_t* temp = searchASL(semAdd);
  	/* semAdd is in ASL case */
  	if(temp->s_next->s_semAdd == semAdd){
  		return(headProcQ(temp->s_next->s_procQ));	/* returns NULL if empty and head otherwise */
  	}
  	/* semAdd is not in ASL case */
  	else{
  		return(NULL);
  	}
}

HIDDEN semd_t* allocSemd()
/* Return NULL if the semdFree list is empty. Otherwise, remove
an element from the semdFree list, provide initial values for ALL
of the semd's fields, and then return a pointer to the removed
element.
*/
{
	semd_t* temp = semdFree_h;
	/* empty list case */
	if(temp == NULL){
		return(NULL);
	}
	/* non-empty list case */
	else {
		semdFree_h = temp->s_next;			/* take temp off free list */
		/* initialize all semd_t fields */
		temp->s_next = NULL;
		temp->s_semAdd = 0;
		temp->s_procQ = NULL;
	}
	return(temp);
}

HIDDEN void freeSemd(semd_t* s)
/* Insert the semaphore pointed to by s onto the semdFree list. */
{
	s->s_next = semdFree_h;
	semdFree_h = s;
}

HIDDEN semd_t* searchASL(int *semAdd)
/* Search the sorted ASL for the node before the semAdd given or for
the node before where it would be if it were in the ASL. Returns this
"parent"/"preceding" node
*/
{
	semd_t* current = semd_h;
	/* loop through the ASL until the next semAdd is larger than the one we want */
	while(current->s_next->s_semAdd < semAdd){
		current = current->s_next;
	}
	return current;
}
