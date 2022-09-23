
#include "FreeRTOS.h"

TCB_t * volatile pxCurrentTCB = NULL;

List_t ReadyList[ configMAX_PRIORITIES ];

void initReadyList( void ) {
	StackType_t i;
	for ( i=0; i<configMAX_PRIORITIES; i++ ) 
		vListInitialise( ( List_t * ) &(ReadyList[ i ]) );
}


void createTask( TCB_t * const pxTCB 
							, StackType_t * const StackBottom 
							, StackType_t stackSize
							, TaskFunction_t pFun) {
	StackType_t *StackTop;
	StackTop = (StackType_t *)( StackBottom + stackSize );
	StackTop = (StackType_t *)( ( StackType_t ) StackTop & ( ~((StackType_t)(7U)) ) );
							
	vListInitialiseItem( ( ListItem_t * ) ( &( pxTCB->ListItem ) ) );
	pxTCB->ListItem.pvOwner = ( TCB_t * ) pxTCB;

	pxTCB->xTicksToDelay = 0U;
	pxTCB->stackTop = pxPortInitialiseStack(StackTop, pFun, NULL);
}
							

/********************************* 空闲任务 *************************************/
TaskHandle_t TaskIdle_Handle;
#define TASKIdle_STACK_SIZE                    20
StackType_t TaskIdleStack[TASKIdle_STACK_SIZE];
TCB_t TaskIdleTCB;
void taskIdle( void *p_arg ) {
	for ( ;; );
}
/*********************************************************************************/


void vTaskSwitchContext( void ) {    
	StackType_t i,j;
	ListItem_t *pxItem;
	for ( i=0; i<configMAX_PRIORITIES; i++ ) {
		pxItem = ( ListItem_t * ) listGET_HEAD_ENTRY( &ReadyList[ i ] );
		for ( j=0; j<ReadyList[ i ].uxNumberOfItems; j++ ) {
			if ( ( ( TCB_t * )pxItem->pvOwner )->xTicksToDelay > 0 ) {
				pxItem = ( ListItem_t * ) pxItem->pxNext;
			} else {
				pxCurrentTCB = ( TCB_t * ) pxItem->pvOwner;
				return;
			}
		}
	}
	pxCurrentTCB = ( TCB_t * ) &TaskIdleTCB;
}	

			

void vTaskStartScheduler( void ){
	//创建空闲任务
	StackType_t *StackTop;
	StackTop = (StackType_t *)( TaskIdleStack + TASKIdle_STACK_SIZE );
	StackTop = (StackType_t *)( ( StackType_t ) StackTop & ( ~((StackType_t)(7U)) ) );
	TaskIdleTCB.stackTop = pxPortInitialiseStack(StackTop, taskIdle, NULL);
	
	vListInitialiseItem( ( ListItem_t * ) &TaskIdleTCB.ListItem );
	TaskIdleTCB.ListItem.pvOwner = ( TCB_t * ) &TaskIdleTCB;
	TaskIdleTCB.xTicksToDelay = 0U;
	
	vListInsertEnd( ( List_t * ) &ReadyList[ configMAX_PRIORITIES - 1 ], &TaskIdleTCB.ListItem );
	TaskIdleTCB.ListItem.xItemValue = portMAX_DELAY;

	pxCurrentTCB = &TaskIdleTCB;
	xPortStartScheduler();
}


void xTaskIncrementTick( void ) {
	StackType_t i,j;
	ListItem_t *pxItem;
	for ( i=0; i<configMAX_PRIORITIES; i++ ) {
		pxItem = ( ListItem_t * ) listGET_HEAD_ENTRY( &ReadyList[ i ] );
		for ( j=0; j<ReadyList[ i ].uxNumberOfItems; j++ ) {
			if ( ( ( TCB_t * )pxItem->pvOwner )->xTicksToDelay > 0 )
				( ( TCB_t * )pxItem->pvOwner )->xTicksToDelay--;
			pxItem = ( ListItem_t * ) pxItem->pxNext;
		}
	}
	taskYIELD();
}



void vTaskDelay( const TickType_t xTicksToDelay )	{
	pxCurrentTCB->xTicksToDelay = xTicksToDelay;
	taskYIELD();
}

