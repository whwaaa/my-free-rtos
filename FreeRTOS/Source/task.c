
#include "FreeRTOS.h"

/********************************* 函数声明 *************************************/
static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB );
static void prvResetNextTaskUnblockTime( void );
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


void initReadyList( void ) {
	StackType_t i;
	for ( i=0; i<configMAX_PRIORITIES; i++ ) 
		vListInitialise( ( List_t * ) &(ReadyList[ i ]) );

	pxDelayedTaskList = &xDelayedTaskList1;
	pxOverflowDelayedTaskList = &xDelayedTaskList2;
	
	vListInitialise( pxDelayedTaskList );
	vListInitialise( pxOverflowDelayedTaskList );
	
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
							
	vListInitialiseItem( ( ListItem_t * ) ( &( pxTCB->ListItem ) ) );
								
	pxTCB->ListItem.pvOwner = ( TCB_t * ) pxTCB;
								
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

void vTaskSwitchContext( void ) {   

	taskSELECT_HIGHEST_PRIORITY_TASK();
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
	return xSwitchRequired;
	
//	StackType_t i,j;
//	ListItem_t *pxItem;
//	
//	const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;
//	xTickCount = xConstTickCount;
//	
//	for ( i=0; i<configMAX_PRIORITIES; i++ ) {
//		pxItem = ( ListItem_t * ) listGET_HEAD_ENTRY( &ReadyList[ i ] );
//		for ( j=0; j<ReadyList[ i ].uxNumberOfItems; j++ ) {
//			if ( ( ( TCB_t * )pxItem->pvOwner )->xTicksToDelay > 0 )
//				( ( TCB_t * )pxItem->pvOwner )->xTicksToDelay--;
//			pxItem = ( ListItem_t * ) pxItem->pxNext;
//		}
//	}
//	taskYIELD();
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
	if ( ( List_t * ) ( &( ( ( TCB_t * ) xTask )->ListItem.pvContainer ) ) == &xSuspendedTaskList ) {
		return pdTRUE;
	} else {
		return pdFALSE;
	}
}

/*------------------------------------- 任务挂起 -------------------------------------*/
void vTaskSuspend( TaskHandle_t xTaskToSuspend ) {
	TCB_t * pxTCB;
	
	//根据句柄获转换成任务TCB
	//pxTCB = ( ( xTaskToSuspend == NULL ) ? ( TCB_t * ) pxCurrentTCB : ( TCB_t * ) ( xTaskToSuspend ) );
	pxTCB = prvGetTCBFromHandle( xTaskToSuspend );
	//将任务从就绪、延时链表中移除
	if ( uxListRemove( &( pxTCB->ListItem ) ) != pdFALSE ) {
		//更新最高就绪优先级
		portRESET_READY_PRIORITY( ( pxTCB->uxPriority ), ( uxTopReadyPriority ) );	
	}
	if ( pxTCB != pxCurrentTCB ) {//就绪、阻塞任务挂起
		prvResetNextTaskUnblockTime();
	} else {//当前任务挂起
		//切换任务
		taskYIELD();
	}
}
/*------------------------------------------------------------------------------------*/

/*------------------------------------- 任务恢复 -------------------------------------*/
void vTaskResume( TaskHandle_t xTaskToResume ) {
	TCB_t * pxTCB;
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
			if ( pxTCB->uxPriority > pxCurrentTCB->uxPriority ) {
				//切换任务
				taskYIELD();
			}
		}
	}
}
/*------------------------------------------------------------------------------------*/
#endif





