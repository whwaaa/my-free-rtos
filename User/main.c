/***********************************************
*	@Discription main
* @Auther whw
* @Date 2022/08/20
************************************************/

#include "freeRTOS.h"

/*
*************************************************************************
*                        任务控制块 & STACK 
*************************************************************************
*/
TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE                    128
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_t Task1TCB;

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE                    128
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_t Task2TCB;

TaskHandle_t Task3_Handle;
#define TASK3_STACK_SIZE                    128
StackType_t Task3Stack[TASK3_STACK_SIZE];
TCB_t Task3TCB;

portCHAR flag1;
portCHAR flag2;
portCHAR flag3;


/*
*************************************************************************
*                               函数声明 
*************************************************************************
*/
void delay (uint32_t count);
void Task1_Entry( void *p_arg );
void Task2_Entry( void *p_arg );
void Task3_Entry( void *p_arg );


/*
************************************************************************
*                                main函数
************************************************************************
*/
int main(void){
	     
	createTask( &Task1TCB, Task1Stack, TASK1_STACK_SIZE, Task1_Entry, ( UBaseType_t ) 2 );
	createTask( &Task2TCB, Task2Stack, TASK2_STACK_SIZE, Task2_Entry, ( UBaseType_t ) 2 );
	createTask( &Task3TCB, Task3Stack, TASK3_STACK_SIZE, Task3_Entry, ( UBaseType_t ) 3 );

	vTaskStartScheduler();
}




void delay( StackType_t count ) {
	for ( ; count > 0 ; count-- );
}



/* 任务1 */
void Task1_Entry( void *p_arg )
{
	for( ;; )
	{
		flag1 = 1;
		delay( 100 );		
		//vTaskDelay( 10 );		
		flag1 = 0;
		delay( 100 );
		//vTaskDelay( 10 );
	}
}

/* 任务2 */
void Task2_Entry( void *p_arg )
{
	for( ;; )
	{
		flag2 = 1;
		delay( 100 );
		//vTaskDelay( 10 );				
		flag2 = 0;
		delay( 100 );
		//vTaskDelay( 5 );		
	}
}

 
void Task3_Entry( void *p_arg )
{
	for( ;; )
	{
		flag3 = 1;
    vTaskDelay( 1 );	
		flag3 = 0;
    vTaskDelay( 1 );
	}
}




