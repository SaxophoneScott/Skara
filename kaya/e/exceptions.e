#ifndef EXCEPTIONS
#define EXCEPTIONS

/************************** EXCEPTIONS.E ******************************
*
*  The externals declaration file for the Exceptions 
*    Module.
*
*  Written by Scott Harrington and Kara Schatz
*/

#include "../h/types.h"

extern  void SyscallHandler();
extern  void ProgramTrapHandler();
extern  void TLBManagementHandler();
extern  void LoadState(state_PTR processState);
extern  void IncrementProcessTime(pcb_PTR process);



/***************************************************************/

#endif
