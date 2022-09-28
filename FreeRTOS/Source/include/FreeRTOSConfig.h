#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_16_BIT_TICKS		0

#define xPortPendSVHandler   PendSV_Handler
#define vPortSVCHandler      SVC_Handler
#define xPortSysTickHandler  SysTick_Handler

#define configCPU_CLOCK_HZ				( ( unsigned long ) 12000000 )	
#define configTICK_RATE_HZ				( ( TickType_t ) 100 )

#define configMAX_PRIORITIES		5 //就绪列表优先级


#define configKERNEL_INTERRUPT_PRIORITY 		255   /* 高四位有效，即等于0xff，或者是15 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* 高四位有效，即等于0xb0，或者是11 */

//任务挂起
#define INCLUDE_vTaskSuspend	1

/**********************************************************************
***********************************************************************
 									FreeRTOS 基础配置配置选项
***********************************************************************/
/* 置 1：RTOS 使用抢占式调度器；置 0：RTOS 使用协作式调度器（时间片）
*
* 注：在多任务管理机制上，操作系统可以分为抢占式和协作式两种。
* 协作式操作系统是任务主动释放 CPU 后，切换到下一个任务。
* 任务切换的时机完全取决于正在运行的任务。
*/
#define configUSE_PREEMPTION 1

#define INCLUDE_vTaskDelete 0

#endif /* FREERTOS_CONFIG_H */
