#ifndef INITPROC
#define INITPROC

/************************** INITPROC.E ******************************
*
*  The externals declaration file for the InitProc Module.
*
*  Written by Scott Harrington and Kara Schatz
*/

#include "../h/types.h"
#include "../h/const.h"

extern int					masterSema4;
extern int					swapmutex; 
extern int					deviceSema4s[DEVICECOUNT]; 
/* structs */
extern segtable_t				segTable[PROCCNT];
extern ospagetable_t*				ksegosPT;
extern kupagetable_t*				kuseg3PT;
extern upcb_t					userProcArray[PROCCNT];
extern frameswappoole_t				frameSwapPool[POOLSIZE];

extern void test();
extern void allowInterrupts(int on);
extern unsigned int getTapeBufferAddr(int tapeNum);
extern unsigned int getDiskBufferAddr(int diskNum);
extern unsigned int getStackPageAddr(int procNum, int exceptionType);
extern int getCylinderNum(int pageNum);
extern int getSectorNum(int procNum);
extern int getHeadNum(int segment);


/***************************************************************/

#endif
