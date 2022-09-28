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

TaskHandle_t Task4_Handle;
#define TASK4_STACK_SIZE                    128
StackType_t Task4Stack[TASK4_STACK_SIZE];
TCB_t Task4TCB;

TaskHandle_t Task5_Handle;
#define TASK5_STACK_SIZE                    128
StackType_t Task5Stack[TASK5_STACK_SIZE];
TCB_t Task5TCB;

portCHAR flag1;
portCHAR flag2;
portCHAR flag3;
portCHAR flag4;
portCHAR flag5;


/*
*************************************************************************
*                               函数声明
*************************************************************************
*/
void delay (uint32_t count);
void Task1_Entry( void *p_arg );
void Task2_Entry( void *p_arg );
void Task3_Entry( void *p_arg );
void Task4_Entry( void *p_arg );
void Task5_Entry( void *p_arg );
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;
TaskHandle_t Task5;



/*
************************************************************************
*                                main函数
************************************************************************
*/
int main(void) {

    createTask( &Task1TCB, Task1Stack, TASK1_STACK_SIZE, Task1_Entry, ( UBaseType_t ) 2, &Task1);
    createTask( &Task2TCB, Task2Stack, TASK2_STACK_SIZE, Task2_Entry, ( UBaseType_t ) 2, &Task2);
    createTask( &Task3TCB, Task3Stack, TASK3_STACK_SIZE, Task3_Entry, ( UBaseType_t ) 2, &Task3);
		createTask( &Task4TCB, Task4Stack, TASK4_STACK_SIZE, Task4_Entry, ( UBaseType_t ) 2, &Task4);
		createTask( &Task5TCB, Task5Stack, TASK5_STACK_SIZE, Task5_Entry, ( UBaseType_t ) 3, &Task5);
	
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
				flag1 = ~flag1;
				delay( 100 );
    }
}

/* 任务2 */
void Task2_Entry( void *p_arg )
{
    for( ;; )
    {
				flag2 = ~flag2;
				delay( 100 );
    }
}

void Task3_Entry( void *p_arg )
{
    for( ;; )
    {
			flag3 = ~flag3;
//        flag3 = 1;
//        vTaskDelay( 1 );
//        flag3 = 0;
//        vTaskDelay( 1 );
			taskENTER_CRITICAL();
			vTaskResume( Task4 );
			vTaskSuspend( NULL );
			portEXIT_CRITICAL();
    }
}
void Task4_Entry( void *p_arg )
{
    for( ;; )
    {
//        flag4 = 1;
//        vTaskDelay( 1 );
//        flag4 = 0;
//        vTaskDelay( 1 );
			taskENTER_CRITICAL();
			flag4 = ~flag4;
			vTaskResume( Task3 );
			vTaskSuspend( NULL );
			portEXIT_CRITICAL();
    }
}

/* 任务5 */
void Task5_Entry( void *p_arg )
{
    for( ;; )
    {
				flag5 = ~flag5;
				vTaskDelay( 10 );
    }
}



