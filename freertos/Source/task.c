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
#include "FreeRTOS.h"

/* ��ǰ�������е������������ƿ�ָ�룬Ĭ�ϳ�ʼ��ΪNULL */
TCB_t * volatile pxCurrentTCB = NULL;

/* ��������б�һ��List_t��������ڵ㣩���飬�±��Ӧ��������ȼ������֧��256�����ȼ� */
List_t pxReadyTasksLists[configMAX_PRIORITIES];



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


/* �������������� */
void vTaskStartScheduler( void ){
	// �ֶ�ָ����һ�����е�����
	pxCurrentTCB = &Task1TCB;
	
	// ����������
	xPortStartScheduler();
	
}


/* �����л���������PendSV�жϷ������е��� */
void vTaskSwitchContext(void){
	/* �ݲ�֧�����ȼ���ֻ������������������л� */
	if(pxCurrentTCB == &Task1TCB){
		pxCurrentTCB = &Task2TCB;
	
	}
	else{
		pxCurrentTCB = &Task1TCB;
	}



}
