#ifndef __TASK__H
#define __TASK__H

#include "freeRTOS.h"
#include "list.h"

//不带保护版
#define taskENTER_CRITICAL()               portENTER_CRITICAL()
#define taskEXIT_CRITICAL()                portEXIT_CRITICAL()
//带保护版
#define taskENTER_CRITICAL_FROM_ISR()      portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL_FROM_ISR( x )    portCLEAR_INTERRUPT_MASK_FROM_ISR( x )

#define tskIDLE_PRIORITY			       ( ( UBaseType_t ) 0U )

typedef void (*TaskFunction_t)( void * );
typedef void * TaskHandle_t;

void xPortSysTickHandler( void );

typedef struct {
	StackType_t* stackTop;
	ListItem_t ListItem;
	TickType_t xTicksToDelay;
	UBaseType_t uxPriority;
}TCB_t;


#define taskYIELD()			portYIELD()

extern List_t ReadyList[ configMAX_PRIORITIES ];

void createTask( TCB_t * const pxTCB 
							, StackType_t * const StackBottom 
							, StackType_t stackSize
							, TaskFunction_t pFun
							, UBaseType_t uxPriority
							, TaskHandle_t * xTaskHandle);
							
							
void vTaskStartScheduler( void );
BaseType_t xTaskIncrementTick( void );
void initReadyList( void );
void vTaskDelay( const TickType_t xTicksToDelay );
TickType_t xTaskGetTickCount( void );
							
void vTaskSuspend( TaskHandle_t xTaskToSuspend );
void vTaskResume( TaskHandle_t xTaskToResume );
#endif


