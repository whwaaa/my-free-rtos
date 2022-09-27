
#include "FreeRTOS.h"

/********************************* �������� *************************************/
static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB );
static void prvResetNextTaskUnblockTime( void );
/********************************************************************************/
static void prvResetNextTaskUnblockTime( void );

TCB_t * volatile pxCurrentTCB = NULL;

List_t ReadyList[ configMAX_PRIORITIES ];
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t xSuspendedTaskList;//��������
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
							, UBaseType_t uxPriority/* �������ȼ�����ֵԽ�����ȼ�Խ�� */
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
	
	/* ��������ӵ������б� */
	prvAddNewTaskToReadyList( pxTCB );
}
							



/********************************* �������� *************************************/
TaskHandle_t TaskIdle_Handle;
#define TASKIdle_STACK_SIZE                    20
StackType_t TaskIdleStack[TASKIdle_STACK_SIZE];
TCB_t TaskIdleTCB;
void taskIdle( void *p_arg ) {
	for ( ;; );
}
/*********************************************************************************/

//�л������ʱ����ͷ������ʱ�����ָ�룬�������������µ���ʱ��
#define taskSWITCH_DELAYED_LISTS()		\
{			\
			List_t * tempList = pxOverflowDelayedTaskList; \
			pxOverflowDelayedTaskList = pxDelayedTaskList; \
			pxDelayedTaskList = tempList;	\
			prvResetNextTaskUnblockTime(); \
}			\

#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )
/* ��������ӵ������б� */                                    
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
	//������������
	createTask( &TaskIdleTCB, TaskIdleStack, TASKIdle_STACK_SIZE, taskIdle, ( UBaseType_t ) tskIDLE_PRIORITY, NULL );

	xNextTaskUnblockTime = portMAX_DELAY;
	xTickCount = ( TickType_t ) configINITIAL_TICK_COUNT;
	
	xPortStartScheduler();
}


BaseType_t xTaskIncrementTick( void ) {
	
	TCB_t * pxTCB;
	TickType_t itemValue;
	BaseType_t xSwitchRequired = pdFALSE;
	
	//ϵͳʱ������
	const TickType_t xConstTickCount = xTickCount + ( TickType_t ) 1;
	xTickCount = xConstTickCount;
	//ʱ�����
	if( xConstTickCount == ( UBaseType_t ) 0U ) {
		//1.���������ʱ�������ʱ����ָ�룬2.��������������ʱ��
		taskSWITCH_DELAYED_LISTS();
	}
	
	//��������
	if ( xConstTickCount >= xNextTaskUnblockTime ) {
		for ( ;; ) {
			//ȡ����ʱ����ĵ�һ���������ʱ�Ǹ�����ʱʱ���������ȡ����һ��һ�������µ��ڵ�����
			pxTCB = ( ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList ) );
			itemValue = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxDelayedTaskList );
			//�ж�������ʱ���ڵ������Ѿ����Ƴ�
			if ( xConstTickCount < itemValue ) {
				//������������ĵ���ʱ��
				xNextTaskUnblockTime = itemValue;
				break;
			}
			
			//��TCB����ʱ�������Ƴ�
			uxListRemove( &( pxTCB->ListItem ) );
			
			//��ӽ���������
			prvAddTaskToReadyList( pxTCB );
			//vListInsertEnd( ReadyList, &( pxTCB->ListItem ) );
			
			//�ж��Ƿ��л�����
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
	//������Ӿ����������Ƴ�
	if ( uxListRemove( ( ListItem_t * ) &( pxTCB->ListItem ) ) == ( UBaseType_t ) 0U ) {
		//��Ӧ���ȼ�λ��0
		portRESET_READY_PRIORITY( pxTCB->uxPriority , uxTopReadyPriority );
	}
	//��xTickCount + xTicksToDelay��ֵ��Ϊ�����ʱ���������ֵ��ӽ���ʱ����
	xTaskTick = xTickCount + xTicksToWait;
	pxTCB->ListItem.xItemValue = xTaskTick;
	
	if ( xTaskTick < xTickCount ) {//�������ӽ������ʱ����
		vListInsert( pxOverflowDelayedTaskList, ( ListItem_t * ) &( pxTCB->ListItem ) );
	} else {
		vListInsert( pxDelayedTaskList, ( ListItem_t * ) &( pxTCB->ListItem ) );
		//����xNextTaskUnblockTime��ֵ
		if ( xNextTaskUnblockTime > xTaskTick ) {
			xNextTaskUnblockTime = xTaskTick;
		}
	}
}


void vTaskDelay( const TickType_t xTicksToDelay )	{
	//TCB_t * pxTCB = pxCurrentTCB;
	//pxCurrentTCB->xTicksToDelay = xTicksToDelay;
	
	prvAddCurrentTaskToDelayedList( xTicksToDelay );
	
	//�л�����
	taskYIELD();
}

static void prvResetNextTaskUnblockTime( void ) {
	if ( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE ) {
		xNextTaskUnblockTime = portMAX_DELAY;
	} else {
		xNextTaskUnblockTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxDelayedTaskList );
	}
}


/* ��������ӵ������б� */
static void prvAddNewTaskToReadyList( TCB_t * pxNewTCB ){
	/* ��������ӵ������б� */
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
		/* ��������ӵ������б� */
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

/*------------------------------------- ������� -------------------------------------*/
void vTaskSuspend( TaskHandle_t xTaskToSuspend ) {
	TCB_t * pxTCB;
	
	//���ݾ����ת��������TCB
	//pxTCB = ( ( xTaskToSuspend == NULL ) ? ( TCB_t * ) pxCurrentTCB : ( TCB_t * ) ( xTaskToSuspend ) );
	pxTCB = prvGetTCBFromHandle( xTaskToSuspend );
	//������Ӿ�������ʱ�������Ƴ�
	if ( uxListRemove( &( pxTCB->ListItem ) ) != pdFALSE ) {
		//������߾������ȼ�
		portRESET_READY_PRIORITY( ( pxTCB->uxPriority ), ( uxTopReadyPriority ) );	
	}
	if ( pxTCB != pxCurrentTCB ) {//�����������������
		prvResetNextTaskUnblockTime();
	} else {//��ǰ�������
		//�л�����
		taskYIELD();
	}
}
/*------------------------------------------------------------------------------------*/

/*------------------------------------- ����ָ� -------------------------------------*/
void vTaskResume( TaskHandle_t xTaskToResume ) {
	TCB_t * pxTCB;
	//�ָ��������ܿ����ǵ�ǰ����ǰ�����ڹ���״̬
	if ( xTaskToResume!=NULL && xTaskToResume!=( TaskHandle_t ) pxCurrentTCB ) {
		//ȷ�������ǹ���״̬
		if ( prvTaskIsTaskSuspended( xTaskToResume ) != pdFALSE ) {
			pxTCB = ( TCB_t * ) xTaskToResume;
			//�ӹ��������Ƴ�
			uxListRemove( &( pxTCB->ListItem ) );
			//��ӽ���������������߾������ȼ���
			vListInsertEnd( &( ReadyList[ pxTCB->uxPriority ] ), &( pxTCB->ListItem ) );
			//���ָ����������ȼ��Ƿ�ȵ�ǰ�ߣ�����ִ�������л�
			if ( pxTCB->uxPriority > pxCurrentTCB->uxPriority ) {
				//�л�����
				taskYIELD();
			}
		}
	}
}
/*------------------------------------------------------------------------------------*/
#endif





