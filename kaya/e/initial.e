#ifndef INITIAL
#define INITIAL

/************************** INITIAL.e******************************
*
*  The externals declaration file for Initial.c
*    Module.
*
*  Written by Scott Harrington and Kara Schatz
*/

#include "../h/types.h"

extern int processCount;
extern int softBlockCount;
extern pcb_PTR currentProcess;
extern pcb_PTR readyQ;
extern int semaphoreArray[SEMCOUNT]; 
extern cpu_t processStartTime;

extern void main();
/***************************************************************/

#endif
