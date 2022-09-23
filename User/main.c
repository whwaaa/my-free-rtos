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
#define TASK1_STACK_SIZE                    20
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_t Task1TCB;

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE                    20
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_t Task2TCB;

StackType_t flag1;
StackType_t flag2;


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
		flag1 = 0;
		delay( 100 );
		
		/* 任务切换，这里是手动切换 */
    taskYIELD();
	}
}

/* 任务2 */
void Task2_Entry( void *p_arg )
{
	for( ;; )
	{
		flag2 = 1;
		delay( 100 );				
		flag2 = 0;
		delay( 100 );		
		
		/* 任务切换，这里是手动切换 */
    taskYIELD();
	}
}



int main(void){
	
	createTask(&Task1TCB, Task1Stack, TASK1_STACK_SIZE, Task1_Entry);
	createTask(&Task2TCB, Task2Stack, TASK2_STACK_SIZE, Task2_Entry);

	vTaskStartScheduler();
}










