/*!
* \file task.h
* \brief ������ͷ�ļ�
* 
*��ϸ���� 
* 
* \author zbaofu
* \version V1.1
* \date 20230811 
*
*/

#ifndef TASK_H
#define TASK_H

#include "portmacro.h"
#include "FreeRTOSConfig.h"
#include "projdefs.h"
#include "FreeRTOS.h"
#include "portable.h"


#define tskIDLE_PRIORITY			 ( ( UBaseType_t ) 0U )
#define taskYIELD()             portYIELD()



/* ����������ƿ� */
typedef struct tskTaskControlBlock{
	volatile StackType_t *pxTopOfStack;  // ջ��
	ListItem_t            xStateListItem; // ����ڵ�
	StackType_t           *pxStack; // ����ջ��ʼ��ַ
	char                  pcTaskName[configMAX_TASK_NAME_LEN]; // ��������
	TickType_t            xTicksToDelay; /* ������ʱ */
	UBaseType_t           uxPriority;  /* �������ȼ� */
	
} tskTCB;

typedef tskTCB TCB_t;

/* ������ */
typedef void * TaskHandle_t;



/* ��̬���񴴽����� */
#if (configSUPPORT_STATIC_ALLOCATION==1) 
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,  // ������ڣ�����ĺ�������
															const char * const pcName,   // ��������
															const uint32_t ulStackDepth,  // ����ջ��С
															void *const pvParameters,   // �����β�
															UBaseType_t uxPriority,      // �������ȼ�
															StackType_t* const puxStackBuffer, // ����ջ��ʼ��ַ
															TCB_t* const pxTaskBuffer);  // ������ƿ�ָ��

#endif

/* ���������� */
static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,              /* ������� */
									const char * const pcName,              /* �������ƣ��ַ�����ʽ */
									const uint32_t ulStackDepth,            /* ����ջ��С����λΪ�� */
									void * const pvParameters,              /* �����β� */
									UBaseType_t uxPriority,                 /* �������ȼ�����ֵԽ�����ȼ�Խ�� */
									TaskHandle_t * const pxCreatedTask,     /* ������ */
									TCB_t *pxNewTCB ) ;
static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB );		// ��������񵽾����б�													
void prvInitialiseTaskLists( void );   // ��ʼ����������б�                             
void vTaskStartScheduler( void );    // ����������
void vTaskSwitchContext( void );		 // �����л�����
static portTASK_FUNCTION( prvIdleTask, pvParameters ); // ����������ִ�к��� 
void vTaskDelay( const TickType_t xTicksToDelay ); // ������ʱ����
void xTaskIncrementTick(void);  // ����ϵͳʱ��


/* ���жϱ����Ľ����ٽ�εĺ� */									
#define taskENTER_CRITICAL()           portENTER_CRITICAL()
#define taskEXIT_CRITICAL()            portEXIT_CRITICAL() 
									
			
/* ���жϱ����Ľ����ٽ�εĺ� */
#define taskENTER_CRITICAL_FROM_ISR()  portSET_INTERRUPT_MASK_FROM_ISR()																		
#define taskEXIT_CRITICAL_FROM_ISR( x )   portCLEAR_INTERRUPT_MASK_FROM_ISR( x )									
	
									

#endif
