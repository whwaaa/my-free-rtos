#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_16_BIT_TICKS		0

#define xPortPendSVHandler   PendSV_Handler
#define vPortSVCHandler      SVC_Handler
#define xPortSysTickHandler  SysTick_Handler

#define configCPU_CLOCK_HZ				( ( unsigned long ) 12000000 )	
#define configTICK_RATE_HZ				( ( TickType_t ) 100 )

#define configMAX_PRIORITIES		5 //就绪列表优先级


#endif /* FREERTOS_CONFIG_H */
