#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

portCHAR flag1;

void delay( StackType_t count ) {
	for ( ; count > 0 ; count-- );
}

/* ����1 */
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

/**
  * @brief  ������
  * @param  ��  
  * @retval ��
  */
int main ( void )
{	
	
  vTaskStartScheduler();	

}

