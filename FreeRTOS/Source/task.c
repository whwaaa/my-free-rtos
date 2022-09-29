
#include "FreeRTOS.h"

/********************************* 函数声明 *************************************/
static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB );
static void prvResetNextTaskUnblockTime( void );
#if ( INCLUDE_vTaskDelete == 1 )
static void prvCheckTasksWaitingTermination( void );
#endif
/********************************************************************************/
static void prvResetNextTaskUnblockTime( void );

TCB_t * volatile pxCurrentTCB = NULL;

List_t ReadyList[ configMAX_PRIORITIES ];
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t xSuspendedTaskList;//挂起链表
static List_t * volatile pxDelayedTaskList;
static List_t * volatile pxOverflowDelayedTaskList;


static volatile TickType_t xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
static volatile UBaseType_t uxTopReadyPriority = ( UBaseType_t ) 0U;
static UBaseType_t uxTaskNumber 					= ( UBaseType_t ) 0U;
static volatile TickType_t xNextTaskUnblockTime = ( TickType_t ) 0U;
static volatile UBaseType_t uxCurrentNumberOfTasks 	= ( UBaseType_t ) 0U;
static volatile BaseType_t uxSchedulerSuspended = pdFALSE;//所有任务被挂起
static volatile BaseType_t uxPendedTicks = ( BaseType_t ) 0U;//延迟时基
static volatile BaseType_t xYieldPending = pdFALSE;//有带调度任务



#if ( INCLUDE_vTaskDelete == 1 )
static List_t xTasksWaitingTermination;//待终止链表
static volatile UBaseType_t uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;
#endif

void initReadyList( void ) {
	StackType_t i;
	for ( i=0; i<configMAX_PRIORITIES; i++ ) 
		vListInitialise( ( List_t * ) &(ReadyList[ i ]) );

	pxDelayedTaskList = &xDelayedTaskList1;
	pxOverflowDelayedTaskList = &xDelayedTaskList2;
	
	vListInitialise( pxDelayedTaskList );
	vListInitialise( pxOverflowDelayedTaskList );
	
	vListInitialise( &xSuspendedTaskList );//???????
	
}


void createTask( TCB_t * const pxTCB 
							, StackType_t * const StackBottom 
							, StackType_t stackSize
							, TaskFunction_t pFun
							, UBaseType_t uxPriority/* 任务优先级，数值越大，优先级越高 */
							, TaskHandle_t * const pxTaskHandle
							) {
	StackType_t *StackTop;
	StackTop = (StackType_t *)( StackBottom + stackSize );
	StackTop = (StackType_t *)( ( StackType_t ) StackTop & ( ~((StackType_t)(7U)) ) );
							
	//vListInitialiseItem( ( ListItem_t * ) ( &( pxTCB->ListItem ) ) );
								
	pxTCB->ListItem.pvOwner = ( TCB_t * ) pxTCB;
	pxTCB->stackBottom = StackBottom;
								
	if ( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES ) {
		uxPriority = configMAX_PRIORITIES;
	}
	pxTCB->uxPriority = uxPriority;

	pxTCB->xTicksToDelay = 0U;
	pxTCB->stackTop = pxPortInitialiseStack(StackTop, pFun, NULL);
								
	*pxTaskHandle = ( TaskHandle_t * ) pxTCB;
	
	/* 将任务添加到就绪列表 */
	prvAddNewTaskToReadyList( pxTCB );
}
							



/********************************* 空闲任务 *************************************/
TaskHandle_t TaskIdle_Handle;
#define TASKIdle_STACK_SIZE                    20
StackType_t TaskIdleStack[TASKIdle_STACK_SIZE];
TCB_t TaskIdleTCB;

void taskIdle( void *p_arg ) {
	#if ( INCLUDE_vTaskDelete == 1 )
	//1.检查是否有待终止任务，有则释放资源
	prvCheckTasksWaitingTermination();
	#endif
	
	//2.检查同等级下有没有其他任务就绪，有直接切换
	if ( listCURRENT_LIST_LENGTH( &ReadyList[ tskIDLE_PRIORITY ] ) > ( UBaseType_t ) 1U ) {
		taskYIELD();//切换其他任务
	}
	for ( ;; );
	
}
/*********************************************************************************/

//切换溢出延时链表和非溢出延时链表的指针，并更新任务最新到期时间
#define taskSWITCH_DELAYED_LISTS()		\
{			\
			List_t * tempList = pxOverflowDelayedTaskList; \
			pxOverflowDelayedTaskList = pxDelayedTaskList; \
			pxDelayedTaskList = tempList;	\
			prvResetNextTaskUnblockTime(); \
}			\

#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )
/* 将任务添加到就绪列表 */                                    
#define prvAddTaskToReadyList( pxTCB )	\
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );	 \
	vListInsertEnd( &( ReadyList[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->ListItem ) ); \

#define taskSELECT_HIGHEST_PRIORITY_TASK(){ \
	UBaseType_t uxTopPriority;	\
	portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority ); \
	listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( ReadyList[ uxTopPriority ] ) ); \
}	\

#define prvGetTCBFromHandle( pxHandle ) \
( ( ( pxHandle ) == NULL ) ? ( TCB_t * ) pxCurrentTCB : ( TCB_t * ) ( pxHandle ) ) \

#define taskRESET_READY_PRIORITY( uxPriority ) \
if( listLIST_IS_EMPTY( &ReadyList[ uxPriority ] ) != pdFALSE ) { \
	portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority ); \
} \


void vTaskSwitchContext( void ) {   
	if( uxSchedulerSuspended != pdFALSE ) {
			xYieldPending = pdFALSE;
			taskSELECT_HIGHEST_PRIORITY_TASK();
	} else {
			xYieldPending = pdTRUE;
	}
}	

			

void vTaskStartScheduler( void ){
	//创建空闲任务
	createTask( &TaskIdleTCB, TaskIdleStack, TASKIdle_STACK_SIZE, taskIdle, ( UBaseType_t ) tskIDLE_PRIORITY, NULL );

	xNextTaskUnblockTime = portMAX_DELAY;
	xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
	
	xPortStartScheduler();
}


BaseType_t xTaskIncrementTick( void ) {
	
	TCB_t * pxTCB;
	TickType_t itemValue;
	BaseType_t xSwitchRequired = pdFALSE;
	
	UBaseType_t uxTopPriority;
	
	if ( uxSchedulerSuspended != pdFALSE ) {
		
		//系统时基自增
		const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;
		xTickCount = xConstTickCount;
		//时基溢出
		if( xConstTickCount == ( UBaseType_t ) 0U ) {
			//1.交换溢出延时链表和延时链表指针，2.更新最新任务到期时间
			taskSWITCH_DELAYED_LISTS();
		}
		
		//有任务到期
		if ( xConstTickCount >= xNextTaskUnblockTime ) {
			for ( ;; ) {
				//取出延时链表的第一个任务，添加时是根据延时时间进行排序，取出第一个一定是最新到期的任务
				pxTCB = ( ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList ) );
				itemValue = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxDelayedTaskList );
				//判断所有延时到期的任务都已经被移除
				if ( xConstTickCount < itemValue ) {
					//更新最新任务的到期时间
					xNextTaskUnblockTime = itemValue;
					break;
				}
				
				//将TCB从延时链表中移除
				uxListRemove( &( pxTCB->ListItem ) );
				
				//添加进就绪链表
				prvAddTaskToReadyList( pxTCB );
				//vListInsertEnd( ReadyList, &( pxTCB->ListItem ) );
				
				//判断是否切换任务
				if ( pxTCB->uxPriority >= pxCurrentTCB->uxPriority ) {
					xSwitchRequired = pdTRUE;
				}
			}
		}
		
		#if ( configUSE_PREEMPTION == 1 )
			portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );
			if ( listCURRENT_LIST_LENGTH( &ReadyList[ uxTopPriority ] ) > ( UBaseType_t ) 1U ) {
				xSwitchRequired = pdTRUE;
			}
		#endif
	} else {
		
		uxPendedTicks++;
	}
	
	#if ( configUSE_PREEMPTION == 1 )
	{
		if( xYieldPending != pdFALSE )
		{
			xSwitchRequired = pdTRUE;
		}
	}
	#endif /* configUSE_PREEMPTION */
	
	
	return xSwitchRequired;
}

static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait ) {
	TCB_t * const pxTCB = pxCurrentTCB;
	TickType_t xTaskTick;
	//将任务从就绪链表中移除
	if ( uxListRemove( ( ListItem_t * ) &( pxTCB->ListItem ) ) == ( UBaseType_t ) 0U ) {
		//对应优先级位清0
		portRESET_READY_PRIORITY( pxTCB->uxPriority , uxTopReadyPriority );
	}
	//将xTickCount + xTicksToDelay的值做为添加延时链表的序列值添加进延时链表
	xTaskTick = xTickCount + xTicksToWait;
	pxTCB->ListItem.xItemValue = xTaskTick;
	
	if ( xTaskTick < xTickCount ) {//溢出，添加进溢出延时链表
		vListInsert( pxOverflowDelayedTaskList, ( ListItem_t * ) &( pxTCB->ListItem ) );
	} else {
		vListInsert( pxDelayedTaskList, ( ListItem_t * ) &( pxTCB->ListItem ) );
		//更新xNextTaskUnblockTime的值
		if ( xNextTaskUnblockTime > xTaskTick ) {
			xNextTaskUnblockTime = xTaskTick;
		}
	}
}


void vTaskDelay( const TickType_t xTicksToDelay )	{
	//TCB_t * pxTCB = pxCurrentTCB;
	//pxCurrentTCB->xTicksToDelay = xTicksToDelay;
	
	prvAddCurrentTaskToDelayedList( xTicksToDelay );
	
	//切换任务
	taskYIELD();
}

static void prvResetNextTaskUnblockTime( void ) {
	if ( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE ) {
		xNextTaskUnblockTime = portMAX_DELAY;
	} else {
		xNextTaskUnblockTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxDelayedTaskList );
	}
}


/* 将任务添加到就绪列表 */
static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB ){
	/* 将任务添加到就绪列表 */
  //prvAddTaskToReadyList( pxNewTCB );
	
	taskENTER_CRITICAL();
	{
		uxCurrentNumberOfTasks++;
	  if ( pxCurrentTCB == NULL ) {
			pxCurrentTCB = pxNewTCB;
			if ( uxCurrentNumberOfTasks == ( UBaseType_t ) 1U ) {
				initReadyList();
			}
		} else {
			if ( pxNewTCB->uxPriority >= pxCurrentTCB->uxPriority ) {
				pxCurrentTCB = pxNewTCB;
			}
		}
		uxTaskNumber++;
		/* 将任务添加到就绪列表 */
    prvAddTaskToReadyList( pxNewTCB );
		
	}
	taskEXIT_CRITICAL();
	
}


#if ( INCLUDE_vTaskSuspend == 1 )
BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask ) {
	if ( ( List_t * ) ( ( ( ( TCB_t * ) xTask )->ListItem.pvContainer ) ) == &xSuspendedTaskList ) {
		return pdTRUE;
	} else {
		return pdFALSE;
	}
}
/*------------------------------------- 任务挂起 -------------------------------------*/
void vTaskSuspend( TaskHandle_t xTaskToSuspend ) {
	TCB_t * pxTCB;
	portENTER_CRITICAL();
	//根据句柄获转换成任务TCB
	//pxTCB = ( ( xTaskToSuspend == NULL ) ? ( TCB_t * ) pxCurrentTCB : ( TCB_t * ) ( xTaskToSuspend ) );
	pxTCB = prvGetTCBFromHandle( xTaskToSuspend );
	//将任务从就绪、延时链表中移除
	if ( uxListRemove( &( pxTCB->ListItem ) ) == ( UBaseType_t ) 0U ) {
		//更新最高就绪优先级
		portRESET_READY_PRIORITY( ( pxTCB->uxPriority ), ( uxTopReadyPriority ) );	
	}
	//更新最新任务延时到期时间
	prvResetNextTaskUnblockTime();
	//就绪、阻塞任务挂起
	vListInsertEnd( &xSuspendedTaskList, &( pxTCB->ListItem ) );
	if ( pxTCB == pxCurrentTCB ) {
		//切换任务
		taskYIELD();
	}
	portEXIT_CRITICAL();
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------- 任务恢复 -------------------------------------*/
void vTaskResume( TaskHandle_t xTaskToResume ) {
	TCB_t * pxTCB;
	portENTER_CRITICAL();
	//恢复的任务不能可能是当前，当前任务不在挂起状态
	if ( xTaskToResume!=NULL && xTaskToResume!=( TaskHandle_t ) pxCurrentTCB ) {
		//确保任务是挂起状态
		if ( prvTaskIsTaskSuspended( xTaskToResume ) != pdFALSE ) {
			pxTCB = ( TCB_t * ) xTaskToResume;
			//从挂起链表移除
			uxListRemove( &( pxTCB->ListItem ) );
			//添加进就绪链表（更新最高就绪优先级）
			vListInsertEnd( &( ReadyList[ pxTCB->uxPriority ] ), &( pxTCB->ListItem ) );
			//检查恢复的任务优先级是否比当前高，高则执行任务切换
			if ( pxTCB->uxPriority >= pxCurrentTCB->uxPriority ) {
				//切换任务
				taskYIELD();
			}
		}
	}
	portEXIT_CRITICAL();
}
/*----------------------------------- 挂起所有任务 -----------------------------------*/
/*------------------------------------------------------------------------------------*/
void vTaskSuspendAll( void ) {
	++uxSchedulerSuspended;
}
/*----------------------------------- 恢复所有任务 -----------------------------------*/
BaseType_t xTaskResumeAll( void ) {
	TCB_t * pxTCB;
	uxSchedulerSuspended--;
	UBaseType_t uxPendedCounts;
	BaseType_t xAlreadyYielded = pdFALSE;
	
	if( uxSchedulerSuspended == ( BaseType_t ) 0U ) {
			while( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) > ( BaseType_t ) 0U ) {
					pxTCB = listGET_OWNER_OF_HEAD_ENTRY( &xSuspendedTaskList );
					uxListRemove( ( ListItem_t * ) &( pxTCB->ListItem ) );
					prvAddTaskToReadyList( pxTCB );
					if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority ) {
							xYieldPending = pdTRUE;
					}
			}
			uxPendedCounts = uxPendedTicks;
			while( uxPendedCounts > ( UBaseType_t ) 0U ){
					if( xTaskIncrementTick() != pdFALSE ) {
							xYieldPending = pdTRUE;
					}
					--uxPendedCounts;
			}
			uxPendedTicks = ( UBaseType_t ) 0U;
			
			if( xYieldPending != pdFALSE ) {
          taskYIELD();
			}
	}
	return xAlreadyYielded;
}
/*------------------------------------------------------------------------------------*/
#endif

#if ( INCLUDE_vTaskDelete == 1 )
/*------------------------------------- 任务删除 -------------------------------------*/
void vTaskDelete( TaskHandle_t xTaskToDelete ) {
	TCB_t * pxTCB;
	
	
	portENTER_CRITICAL();
	
	pxTCB = prvGetTCBFromHandle( xTaskToDelete );

	
	portEXIT_CRITICAL();
}	

static void prvCheckTasksWaitingTermination( void ) {
	BaseType_t xListIsEmpty;
	TCB_t * pxTCB;
	
	portENTER_CRITICAL();
	
	if ( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U ) {
		//检查待终止链表中的任务
		xListIsEmpty = listLIST_IS_EMPTY( &xTasksWaitingTermination );
	}
	
	if ( xListIsEmpty == pdFALSE ) {
		pxTCB = listGET_OWNER_OF_HEAD_ENTRY( &xTasksWaitingTermination );
		//释放任务堆栈

		//释放TCB
	}	
	portEXIT_CRITICAL();
}
/*------------------------------------------------------------------------------------*/
#endif
TickType_t xTaskGetTickCount( void ) {
	TickType_t xTicks;

	/* Critical section required if running on a 16 bit processor. */
	portENTER_CRITICAL();
	{
		xTicks = xTickCount;
	}
	portEXIT_CRITICAL();

	return xTicks;
}



