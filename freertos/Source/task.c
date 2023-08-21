/*!
* \file task.c
* \brief �������
* 
*��ϸ���� 
* 
* \author zbaofu
* \version V1.1
* \date 20230814 
*/

#include "task.h"


/* ��ǰ�������е������������ƿ�ָ�룬Ĭ�ϳ�ʼ��ΪNULL */
TCB_t * volatile pxCurrentTCB = NULL;

/* ��������б�һ��List_t��������ڵ㣩���飬�±��Ӧ��������ȼ������֧��256�����ȼ� */
List_t pxReadyTasksLists[configMAX_PRIORITIES];

static TaskHandle_t xIdleTaskHandle	= NULL; // ����������
static volatile TickType_t xTickCount = ( TickType_t ) 0U;  // ϵͳʱ������



/* ��̬���񴴽����� */
// ��̬��������tcb��ջ�ڴ���Ҫ���ȶ���ã��Ǿ�̬�ڴ棬ɾ������ʱ���ڴ治���ͷ�
#if (configSUPPORT_STATIC_ALLOCATION==1) 
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,  // ������ڣ�����ĺ�������
															const char * const pcName,   // ��������
															const uint32_t ulStackDepth,  // ����ջ��С
															void *const pvParameters,   // �����β�
															StackType_t* const puxStackBuffer, // ����ջ��ʼ��ַ
															TCB_t* const pxTaskBuffer)  // ������ƿ�ָ��
{
	TCB_t *pxNewTCB;
	TaskHandle_t xReturn;  //����������������ָ�������TCB
	
	if((pxTaskBuffer!=NULL)&&(puxStackBuffer!=NULL)){
		pxNewTCB = (TCB_t*)pxTaskBuffer;
		pxNewTCB->pxStack = (StackType_t*) puxStackBuffer;
		
		/* ���������� */
	  prvInitialiseNewTask(pxTaskCode,pcName,ulStackDepth,pvParameters,
		                       &xReturn,pxNewTCB);
	
	}
	else{
		xReturn = NULL;
	
	}
	return xReturn;
																														
}																

#endif

/* ���������� */
static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,              /* ������� */
									const char * const pcName,              /* �������ƣ��ַ�����ʽ */
									const uint32_t ulStackDepth,            /* ����ջ��С����λΪ�� */
									void * const pvParameters,              /* �����β� */
									TaskHandle_t * const pxCreatedTask,     /* ������ */
									TCB_t *pxNewTCB )                       /* ������ƿ�ָ�� */
{
	// ����ջ����ַָ��
	StackType_t *pxTopOfStack;
	UBaseType_t x;
	
	/* ��ȡջ����ַ */
	pxTopOfStack = pxNewTCB->pxStack+(ulStackDepth - (uint32_t)1);
	
	/* ������8�ֽڶ��� */
	pxTopOfStack = (StackType_t*) (((uint32_t)pxTopOfStack)&(~((uint32_t)0x0007)));
	
	/* ����������ִ洢��TCB�� */
	for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
	{
		pxNewTCB->pcTaskName[ x ] = pcName[ x ];

		if( pcName[ x ] == 0x00 )
		{
			break;
		}
	}
	
	/* �������ֵĳ��Ȳ��ܳ���configMAX_TASK_NAME_LEN */
	pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';

	/* ��ʼ��TCB�е�����ڵ� */
	vListInitialiseItem(&(pxNewTCB->xStateListItem));
	
	/* ��������ڵ��ӵ���� */
	listSET_LIST_ITEM_OWNER (&(pxNewTCB->xStateListItem),pxNewTCB);
	
	/*��ʼ������ջ*/
	pxNewTCB->pxTopOfStack = pxPortInitialiseStack(pxTopOfStack,pxTaskCode,pvParameters);
	
	/* ��������ָ��������ƿ� */
	if((void*)pxCreatedTask!=NULL){
		*pxCreatedTask = (TaskHandle_t) pxNewTCB;
	}


}				

/* �����б��ʼ�� */
void prvInitialiseTaskLists(void){
	UBaseType_t uxPriority;
	for(uxPriority = (UBaseType_t) 0U; 
	    uxPriority<(UBaseType_t)configMAX_PRIORITIES;
	    uxPriority++){
		vListInitialise(&(pxReadyTasksLists[uxPriority]));
	}
}
	
// main.c�ж����������ƿ�
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;
extern TCB_t IdleTaskTCB;
void vApplicationGetIdleTaskMemory( TCB_t **ppxIdleTaskTCBBuffer, 
                                    StackType_t **ppxIdleTaskStackBuffer, 
                                    uint32_t *pulIdleTaskStackSize );


/* �������������� */
void vTaskStartScheduler( void ){
	/*************������������****************/
	TCB_t *pxIdleTaskTCBBuffer = NULL;  // ָ����������tcb
	StackType_t *pxIdleTaskStackBuffer = NULL;  // ָ���������ջ��ջ����ַ
	uint32_t ulIdleTaskStackSize;
	
	// ��ȡ����������ڴ�
	vApplicationGetIdleTaskMemory(&pxIdleTaskTCBBuffer, 
	                              &pxIdleTaskStackBuffer,
	                              &ulIdleTaskStackSize);
	
	// ������������
	xIdleTaskHandle = xTaskCreateStatic( (TaskFunction_t) prvIdleTask, 
	                                     (char *)"IDLE",
																				 (uint32_t)ulIdleTaskStackSize,
																			  (void *) NULL,
																					(StackType_t *)pxIdleTaskStackBuffer,
																				(TCB_t *) pxIdleTaskTCBBuffer);
  
  // ������������ӵ���������б�ͷ��Ĭ�����ȼ����
  vListInsertEnd (&(pxReadyTasksLists[0]),&(((TCB_t *)pxIdleTaskTCBBuffer)->xStateListItem));
		
	// �ֶ�ָ����һ�����е�����
	pxCurrentTCB = &Task1TCB;
																				
	/* ��ʼ��ϵͳʱ�������� */
  xTickCount = ( TickType_t ) 0U;
	
	// ����������
  if( xPortStartScheduler() != pdFALSE ){
		
	}

}

/* ��������ִ�к��� */
static portTASK_FUNCTION( prvIdleTask, pvParameters ){
		/* ��ֹ�������ľ��� */
	( void ) pvParameters;
    
    for(;;)
    {
        /* ����������ʱʲô������ */
    }
}


/* �����л���������PendSV�жϷ������е��� */
#if 0
void vTaskSwitchContext(void){
	/* �ݲ�֧�����ȼ���ֻ������������������л� */
	if(pxCurrentTCB == &Task1TCB){
		pxCurrentTCB = &Task2TCB;
	
	}
	else{
		pxCurrentTCB = &Task1TCB;
	}
}
#else
void vTaskSwitchContext(void){
	
	/* ����ǰ�����ǿ�����������ִ������1����2��
	�����ǵ���ʱ�Ƿ����������û���������ִ�п�������	*/	
	if(pxCurrentTCB == &IdleTaskTCB){
		if(Task1TCB.xTicksToDelay == 0){
			pxCurrentTCB = &Task1TCB;
		
		}
		else if(Task2TCB.xTicksToDelay == 0){
			pxCurrentTCB = &Task2TCB;
		}
		else{
			return;
		}
	}
	/*
	����ǰ���ǿ���������������һ������������һ����������ʱ�����л���������
	������ʱ�У����жϵ�ǰ�����Ƿ�����ʱ״̬���ǵĻ����л����������񣬷����л�
	*/
	else{ 
		if(pxCurrentTCB == &Task1TCB){
			if(Task2TCB.xTicksToDelay == 0){
				pxCurrentTCB = &Task2TCB;
			
			}
			else if(pxCurrentTCB->xTicksToDelay != 0){
				pxCurrentTCB = &IdleTaskTCB;
			}
			else{
				return;
			}
			
		}
		else if(pxCurrentTCB == &Task2TCB){
			if(Task1TCB.xTicksToDelay == 0){
				pxCurrentTCB = &Task1TCB;
			}
			else if(pxCurrentTCB->xTicksToDelay != 0){
				pxCurrentTCB = &IdleTaskTCB;
			}
			else{
				return;
			}		
		}	
	}
}

#endif

/* ������ʱ���� */
void vTaskDelay( const TickType_t xTicksToDelay ){
	TCB_t *pxTCB = NULL;
	
	/* ��ȡ��ǰ����tcb */
	pxTCB = pxCurrentTCB;
	
	/* ������ʱʱ�� */
	pxTCB->xTicksToDelay = xTicksToDelay;
	
	/* �����л� */
	taskYIELD();
}

/* ����ϵͳʱ�� */
void xTaskIncrementTick(void){
	TCB_t *pxTCB = NULL;
  BaseType_t i = 0;
	
	/* ����ϵͳʱ��������ʱ����1 */
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;
	
	/* ɨ������б������������xTicksToDelay�������Ϊ0���ͼ�1 */
	for(i = 0;i < configMAX_PRIORITIES; i++){
		pxTCB = (TCB_t*) listGET_OWNER_OF_HEAD_ENTRY(( &pxReadyTasksLists[i] ));
		if(pxTCB->xTicksToDelay > 0){
			pxTCB->xTicksToDelay --;
		}
	}
	
	/* �����л� */
	portYIELD();

}
