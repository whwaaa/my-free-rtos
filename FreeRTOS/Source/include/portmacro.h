#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "stdint.h"
#include "stddef.h"


/* 数据类型重定义 */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif

#define portNVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT		( 1UL << 28UL )
#define portSY_FULL_READ_WRITE		( 15 )


#define portYIELD()																\
{																				\
	/* 触发PendSV，产生上下文切换 */								                \
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;								\
	__dsb( portSY_FULL_READ_WRITE );											\
	__isb( portSY_FULL_READ_WRITE );											\
}

#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities )  uxReadyPriorities |= ( 1UL << uxPriority )
#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities )  uxReadyPriorities &= ~( 1UL << uxPriority )

#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities )   \
	uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )


/* Critical section management. */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portDISABLE_INTERRUPTS()                  vPortRaiseBASEPRI()				//关中断 不可嵌套
#define portENABLE_INTERRUPTS()                   vPortClearBASEPRIFromISR()//开中断 不带保护//vPortSetBASEPRI( 0 )	

#define portSET_INTERRUPT_MASK_FROM_ISR()         ulPortRaiseBASEPRI()	//关中断 可嵌套
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )    vPortSetBASEPRI( x )	//开中断 带保护

#define portENTER_CRITICAL()                      vPortEnterCritical()
#define portEXIT_CRITICAL() 								 			vPortExitCritical()
/*-----------------------------------------------------------*/

//__forceinline:C++提供的强行内联函数，目的是为了提高函数的执行效率（调用函数部分用实际的函数体代码替换）
static __forceinline void vPortRaiseBASEPRI( void ) {
		uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

		__asm {
				msr basepri, ulNewBASEPRI
				dsb
				isb 
		}
}
static __forceinline uint32_t ulPortRaiseBASEPRI( void ) {
		uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

		__asm{
				mrs ulReturn, basepri
				msr basepri, ulNewBASEPRI
				dsb
				isb
		}
		return ulReturn;
}

static __forceinline void vPortSetBASEPRI( uint32_t ulBASEPRI ){
		__asm{
				msr basepri, ulBASEPRI
		}
}

static __forceinline void vPortClearBASEPRIFromISR( void )
{
		__asm{
				msr basepri, # 0
		}
}

#endif /* PORTMACRO_H */
