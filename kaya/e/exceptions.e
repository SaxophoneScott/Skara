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

extern  void SycallHandler();
extern  void ProgramTrapHandler();
extern  void TLBMangementHandler();
extern  void LoadState(memaddr processState);
extern  void IncrementProcessTime(pcb_PTR process);



/***************************************************************/

#endif
