/*!
* \file task.c
* \brief 任务操作
* 
*详细概述 
* 
* \author zbaofu
* \version V1.1
* \date 20230814 
*/

#include "task.h"
#include "FreeRTOS.h"

/* 当前正在运行的任务的任务控制块指针，默认初始化为NULL */
TCB_t * volatile pxCurrentTCB = NULL;

/* 任务就绪列表，一个List_t（链表根节点）数组，下标对应任务的优先级，最大支持256个优先级 */
List_t pxReadyTasksLists[configMAX_PRIORITIES];



/* 静态任务创建函数 */
// 静态创建任务，tcb和栈内存需要事先定义好，是静态内存，删除任务时，内存不能释放
#if (configSUPPORT_STATIC_ALLOCATION==1) 
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,  // 任务入口，任务的函数名称
															const char * const pcName,   // 任务名称
															const uint32_t ulStackDepth,  // 任务栈大小
															void *const pvParameters,   // 任务形参
															StackType_t* const puxStackBuffer, // 任务栈起始地址
															TCB_t* const pxTaskBuffer)  // 任务控制块指针
{
	TCB_t *pxNewTCB;
	TaskHandle_t xReturn;  //定义任务句柄，用于指向任务的TCB
	
	if((pxTaskBuffer!=NULL)&&(puxStackBuffer!=NULL)){
		pxNewTCB = (TCB_t*)pxTaskBuffer;
		pxNewTCB->pxStack = (StackType_t*) puxStackBuffer;
		
		/* 创建新任务 */
	  prvInitialiseNewTask(pxTaskCode,pcName,ulStackDepth,pvParameters,
		                       &xReturn,pxNewTCB);
	
	}
	else{
		xReturn = NULL;
	
	}
	return xReturn;
																														
}																

#endif

/* 创建新任务 */
static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,              /* 任务入口 */
									const char * const pcName,              /* 任务名称，字符串形式 */
									const uint32_t ulStackDepth,            /* 任务栈大小，单位为字 */
									void * const pvParameters,              /* 任务形参 */
									TaskHandle_t * const pxCreatedTask,     /* 任务句柄 */
									TCB_t *pxNewTCB )                       /* 任务控制块指针 */
{
	// 定义栈顶地址指针
	StackType_t *pxTopOfStack;
	UBaseType_t x;
	
	/* 获取栈顶地址 */
	pxTopOfStack = pxNewTCB->pxStack+(ulStackDepth - (uint32_t)1);
	
	/* 向下做8字节对齐 */
	pxTopOfStack = (StackType_t*) (((uint32_t)pxTopOfStack)&(~((uint32_t)0x0007)));
	
	/* 将任务的名字存储在TCB中 */
	for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
	{
		pxNewTCB->pcTaskName[ x ] = pcName[ x ];

		if( pcName[ x ] == 0x00 )
		{
			break;
		}
	}
	
	/* 任务名字的长度不能超过configMAX_TASK_NAME_LEN */
	pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';

	/* 初始化TCB中的链表节点 */
	vListInitialiseItem(&(pxNewTCB->xStateListItem));
	
	/* 设置链表节点的拥有者 */
	listSET_LIST_ITEM_OWNER (&(pxNewTCB->xStateListItem),pxNewTCB);
	
	/*初始化任务栈*/
	pxNewTCB->pxTopOfStack = pxPortInitialiseStack(pxTopOfStack,pxTaskCode,pvParameters);
	
	/* 让任务句柄指向任务控制块 */
	if((void*)pxCreatedTask!=NULL){
		*pxCreatedTask = (TaskHandle_t) pxNewTCB;
	}


}				

/* 就绪列表初始化 */
void prvInitialiseTaskLists(void){
	UBaseType_t uxPriority;
	for(uxPriority = (UBaseType_t) 0U; 
	    uxPriority<(UBaseType_t)configMAX_PRIORITIES;
	    uxPriority++){
		vListInitialise(&(pxReadyTasksLists[uxPriority]));
	}
}
	
// main.c中定义的任务控制块
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;


/* 启动调度器函数 */
void vTaskStartScheduler( void ){
	// 手动指定第一个运行的任务
	pxCurrentTCB = &Task1TCB;
	
	// 启动调度器
	xPortStartScheduler();
	
}


/* 任务切换函数，在PendSV中断服务函数中调用 */
void vTaskSwitchContext(void){
	/* 暂不支持优先级，只进行两个任务的轮流切换 */
	if(pxCurrentTCB == &Task1TCB){
		pxCurrentTCB = &Task2TCB;
	
	}
	else{
		pxCurrentTCB = &Task1TCB;
	}



}
