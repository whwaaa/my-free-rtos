#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_16_BIT_TICKS		0

#define xPortPendSVHandler   PendSV_Handler
#define vPortSVCHandler      SVC_Handler
#define xPortSysTickHandler  SysTick_Handler

#define configCPU_CLOCK_HZ				( ( unsigned long ) 12000000 )	
#define configTICK_RATE_HZ				( ( TickType_t ) 100 )

#define configMAX_PRIORITIES		5 //�����б����ȼ�


#define configKERNEL_INTERRUPT_PRIORITY 		255   /* ����λ��Ч��������0xff��������15 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* ����λ��Ч��������0xb0��������11 */

//�������
#define INCLUDE_vTaskSuspend	1


#endif /* FREERTOS_CONFIG_H */
