
#include "FreeRTOS.h"

TCB_t * volatile pxCurrentTCB = NULL;
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;


void createTask( TCB_t * const pxTCB 
							, StackType_t * const StackBottom 
							, StackType_t stackSize
							, TaskFunction_t pFun) {
	StackType_t *StackTop;
	StackTop = (StackType_t *)( StackBottom + stackSize );
	StackTop = (StackType_t *)( ( StackType_t ) StackTop & ( ~((StackType_t)(7U)) ) );
	pxTCB->stackTop = pxPortInitialiseStack(StackTop, pFun, NULL);
}
							


void vTaskSwitchContext( void ) {    
	if ( pxCurrentTCB != &Task1TCB ) {
		pxCurrentTCB = ( TCB_t * ) &Task1TCB;
	} else {
		pxCurrentTCB = ( TCB_t * ) &Task2TCB;
	}
}	

			

void vTaskStartScheduler( void ){
	pxCurrentTCB = &Task1TCB;
	xPortStartScheduler();
}

