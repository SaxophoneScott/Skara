
#include "../h/const.h"
#include "../h/types.h"
#include "../e/asl.e"
#include "../e/pcb.e"

HIDDEN semd_t *semd_h, *semdFree_h;

static semd_t * allocSemd()
/* Return NULL if the semdFree list is empty. Otherwise, remove
an element from the semdFree list, provide initial values for ALL
of the semd's fields, and then return a pointer to the removed 
element. */
{
	semd_t* temp = semdFree_h;
	if(temp == NULL){
		return(NULL);
	}
	else {
		semdFree_h = temp->s_next;
		temp->s_next = NULL;
		temp->s_semAdd = 0;
		temp->s_procQ = NULL;
	}
	return(temp);
}

static semd_t *searchASL(int *semAdd)
{
	semd_t* current = semd_h;
	while(current->s_next->s_semAdd < semAdd){
		current = current->s_next;
	}
	return current;
}

static void freeSemd(semd_t* s)
/* Insert the sema4 pointed to by s onto the semdFree list. */
{
	s->s_next = semdFree_h;
	semdFree_h = s;
}

	void initASL()
/* Initialize the semdFree list to contain all the elements of the array
static semd t semdTable[MAXPROC]
This method will be only called once during data structure initialization.
*/
{
	int i;
	static semd_t semdTable[MAXPROC];

	semdFree_h = NULL;

	for(i=0; i < MAXPROC; i++){
		freeSemd(&(semdTable[i]));
	}
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

  semd_t* temp= searchASL(semAdd);
  if(temp ->s_next->s_semAdd==semAdd)
    {
      insertProcQ(&(temp->s_next->s_procQ),p);
      return 0;
    }
  else{
    semd_t * temp2 = allocSemd();
    if(temp2 == NULL)
      {
	/*  error error error */
	return 1;
	  
      }
    else{
      temp2->s_semAdd =semAdd;
      temp2->s_procQ = mkEmptyProcQ();
      temp2->s_next= temp->s_next;
      temp->s_next = temp2;
      insertProcQ(&(temp2->s_procQ), p);
      return 0;
      
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
  semd_t * temp= searchASL(semAdd);
  if(*(temp ->s_next->s_semAdd) ==*semAdd)
    {
      pcb_PTR temp2 =removeProcQ(&(temp->s_next->s_procQ));
      if(emptyProcQ(&(*temp->s_next->s_procQ)))
	{
	  semd_t * temp3 = temp->s_next;
	  temp->s_next = temp->s_next->s_next;
	  freeSemd(temp3);
      	}
      
      return temp2;
      
    }
  else
    {
      return NULL;
    }
}

pcb_PTR outBlocked(pcb_PTR p)
/* Remove the ProcBlk pointed to by p from the process queue associated
with p’s semaphore (p" p semAdd) on the ASL. If ProcBlk
pointed to by p does not appear in the process queue associated with
p’s semaphore, which is an error condition, return NULL; otherwise,
return p. */
{
  semd_t * temp= searchASL(p->p_semAdd);
  if(temp->s_next->s_semAdd ==p->p_semAdd)
    {
      pcb_PTR temp2 = outProcQ(&(temp->s_next->s_procQ), p);
      if(emptyProcQ(temp->s_next->s_procQ))
	{
	  semd_t * temp3 = temp->s_next;
	  temp->s_next = temp->s_next->s_next;
	  freeSemd(temp3);
	}

      return temp2;
      
    }
  else{
    return NULL;
}
}
pcb_PTR headBlocked(int *semAdd)
/* Return a pointer to the ProcBlk that is at the head of the process
queue associated with the semaphore semAdd. Return NULL
if semAdd is not found on the ASL or if the process queue associated
with semAdd is empty. */
{
  /* take the node thats potentially before the semAdd, or where it would be in the list*/
  semd_t * temp = searchASL(semAdd);
  /* case 1 and 2 -> we found the semAdd in the ASL*/
  if(temp->s_next->s_semAdd == semAdd)
    {
      /* case 1 ->  empty proccess queue -> return null */
      if(emptyProcQ(temp->s_next->s_procQ)){
	  return NULL;
	}
	/* case 2 -> theres a proccess queue, so we must return the pointer to the head*/
	else {
	  return headProcQ(temp->s_next->s_procQ);
        }
    }
      /*case 3-4 , The active sem is not on the list, so return null */
      else{
	return NULL;
      }
}


