/*!
* \file task.h
* \brief 链表定义头文件
* 
*详细概述 
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



/* 定义任务控制块 */
typedef struct tskTaskControlBlock{
	volatile StackType_t *pxTopOfStack;  // 栈顶
	ListItem_t xStateListItem; // 任务节点
	StackType_t *pxStack; // 任务栈起始地址
	char pcTaskName[configMAX_TASK_NAME_LEN]; // 任务名称
} tskTCB;

typedef tskTCB TCB_t;

/* 任务句柄 */
typedef void * TaskHandle_t;



/* 静态任务创建函数 */
#if (configSUPPORT_STATIC_ALLOCATION==1) 
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,  // 任务入口，任务的函数名称
															const char * const pcName,   // 任务名称
															const uint32_t ulStackDepth,  // 任务栈大小
															void *const pvParameters,   // 任务形参
															StackType_t* const puxStackBuffer, // 任务栈起始地址
															TCB_t* const pxTaskBuffer);  // 任务控制块指针

#endif

/* 创建新任务 */
static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,              /* 任务入口 */
									const char * const pcName,              /* 任务名称，字符串形式 */
									const uint32_t ulStackDepth,            /* 任务栈大小，单位为字 */
									void * const pvParameters,              /* 任务形参 */
									TaskHandle_t * const pxCreatedTask,     /* 任务句柄 */
									TCB_t *pxNewTCB ) ;
															
void prvInitialiseTaskLists( void );                                
void vTaskStartScheduler( void );
void vTaskSwitchContext( void );										
															

#endif
