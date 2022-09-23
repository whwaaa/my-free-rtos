#ifndef __TASK__H
#define __TASK__H

#include "freeRTOS.h"
#include "list.h"

typedef void (*TaskFunction_t)( void * );
typedef void * TaskHandle_t;

void xPortSysTickHandler( void );


typedef struct {
	StackType_t* stackTop;
	ListItem_t ListItem;
	TickType_t xTicksToDelay;
}TCB_t;


#define taskYIELD()			portYIELD()

extern List_t ReadyList[ configMAX_PRIORITIES ];

void createTask( TCB_t * const pxTCB 
							, StackType_t * const StackBottom 
							, StackType_t stackSize
							, TaskFunction_t pFun);
							
							
void vTaskStartScheduler( void );
void xTaskIncrementTick( void );
void initReadyList( void );
void vTaskDelay( const TickType_t xTicksToDelay );
#endif


