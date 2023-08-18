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

#define taskYIELD()  portYIELD()



/* ����������ƿ� */
typedef struct tskTaskControlBlock{
	volatile StackType_t *pxTopOfStack;  // ջ��
	ListItem_t xStateListItem; // ����ڵ�
	StackType_t *pxStack; // ����ջ��ʼ��ַ
	char pcTaskName[configMAX_TASK_NAME_LEN]; // ��������
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
															StackType_t* const puxStackBuffer, // ����ջ��ʼ��ַ
															TCB_t* const pxTaskBuffer);  // ������ƿ�ָ��

#endif

/* ���������� */
static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,              /* ������� */
									const char * const pcName,              /* �������ƣ��ַ�����ʽ */
									const uint32_t ulStackDepth,            /* ����ջ��С����λΪ�� */
									void * const pvParameters,              /* �����β� */
									TaskHandle_t * const pxCreatedTask,     /* ������ */
									TCB_t *pxNewTCB ) ;
															
void prvInitialiseTaskLists( void );                                
void vTaskStartScheduler( void );
void vTaskSwitchContext( void );										
															

#endif
