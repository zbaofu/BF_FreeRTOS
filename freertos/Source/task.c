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


/* 当前正在运行的任务的任务控制块指针，默认初始化为NULL */
TCB_t * volatile pxCurrentTCB = NULL;

/* 任务就绪列表，一个List_t（链表根节点）数组，下标对应任务的优先级，最大支持256个优先级 */
List_t pxReadyTasksLists[configMAX_PRIORITIES];

static volatile UBaseType_t uxCurrentNumberOfTasks 	= ( UBaseType_t ) 0U;  // 全局任务计数器
static UBaseType_t uxTaskNumber 					= ( UBaseType_t ) 0U;
static TaskHandle_t xIdleTaskHandle	= NULL; // 空闲任务句柄
static volatile TickType_t xTickCount = ( TickType_t ) 0U;  // 系统时基变量
static volatile UBaseType_t uxTopReadyPriority 		= tskIDLE_PRIORITY;  // 存储任务优先级的位图表

/* 将任务添加到就绪列表 */                                    
#define prvAddTaskToReadyList( pxTCB )																   \
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												   \
	vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
	

/* 查找最高优先级的就绪任务的通用方法 */
#if(configUSE_PORT_OPTIMISED_TASK_SELECTION ==0)
	#define taskRECORD_READY_PRIORITY(uxPriority) \
	{                                           \
		if((uxPriority) > uxTopReadyPriority){  \
			uxTopReadyPriority  = uxPriority;     \
		}                                        \
	}
	/* 寻找就绪任务列表中优先级最高的组 */
	#define taskSELECT_HIGHEST_PRIORITY_TASK()  \
	{                                           \
		UBaseType_t uxTopPriority = uxTopReadyPriority; \
		/* 从就绪列表中的最高优先级寻找当前链表是否有就绪任务，没有则减一 */  \
		while( listLIST_IS_EMPTY (&(pxReadyTasksLists[uxTopPriority])) )\
		{                                                               \
			--uxTopPriority;                                              \
		}                                                              \	
		/* 获取优先级最高的就绪任务的TCB，更新到pxCurrentTCB */          \
		listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB,&(pxReadyTasksLists[uxTopPriority])); \
		/* 更新最高优先级 */                                              \
		uxTopReadyPriority = uxTopPriority;                            \
	}

	/* 这两个宏定义只有在选择优化方法时才用，这里定义为空 */
	#define taskRESET_READY_PRIORITY( uxPriority )
	#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )
	
/* 查找最高优先级的就绪任务的方法：根据处理器架构优化过的方法 */
#else
	#define taskRECORD_READY_PRIORITY(uxPriority)  portRECORD_READY_PRIORITY(uxPriority, uxTopReadyPriority)
	
	#define taskSELECT_HIGHEST_PRIORITY_TASK()      \
	{                                               \
		/* 寻找最高优先级 */                            \
		UBaseType_t uxTopPriority;                      \
		portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority );								    \
		/* 更新到pxCurrentTCB */                                                             \
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );	 \
	}
	

#if 0
	#define taskRESET_READY_PRIORITY( uxPriority )														\
	{																									\
		if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 )	\
		{																								\
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );							\
		}																								\
	}
#else
    #define taskRESET_READY_PRIORITY( uxPriority )											            \
    {																							        \
            portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority ) );					        \
    }
#endif
	
#endif



/* 静态任务创建函数 */
// 静态创建任务，tcb和栈内存需要事先定义好，是静态内存，删除任务时，内存不能释放
#if (configSUPPORT_STATIC_ALLOCATION==1) 
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,  // 任务入口，任务的函数名称
															const char * const pcName,   // 任务名称
															const uint32_t ulStackDepth,  // 任务栈大小
															void *const pvParameters,   // 任务形参
															UBaseType_t uxPriority,      // 任务优先级
															StackType_t* const puxStackBuffer, // 任务栈起始地址
															TCB_t* const pxTaskBuffer)  // 任务控制块指针
{
	TCB_t *pxNewTCB;
	TaskHandle_t xReturn;  //定义任务句柄，用于指向任务的TCB
	
	if((pxTaskBuffer!=NULL)&&(puxStackBuffer!=NULL)){
		pxNewTCB = (TCB_t*)pxTaskBuffer;
		pxNewTCB->pxStack = (StackType_t*) puxStackBuffer;
		
		/* 创建新任务 */
	  prvInitialiseNewTask(pxTaskCode,pcName,ulStackDepth,pvParameters,uxPriority,
		                       &xReturn,pxNewTCB);
		prvAddNewTaskToReadyList(pxNewTCB); // 将任务添加到就绪列表
	
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
									UBaseType_t uxPriority,                 /* 任务优先级，数值越大，优先级越高 */
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
	
	/* 初始化优先级 */
	if(uxPriority >= (UBaseType_t)configMAX_PRIORITIES){
		uxPriority = (UBaseType_t) configMAX_PRIORITIES - (UBaseType_t)1U;
	}
	pxNewTCB->uxPriority = uxPriority;
	
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

static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{
	/* 进入临界段 */
	taskENTER_CRITICAL();
	{
		/* 全局任务计时器加一操作 */
        uxCurrentNumberOfTasks++;
        
        /* 如果pxCurrentTCB为空，则将pxCurrentTCB指向新创建的任务 */
		if( pxCurrentTCB == NULL )
		{
			pxCurrentTCB = pxNewTCB;

			/* 如果是第一次创建任务，则需要初始化任务相关的列表 */
            if( uxCurrentNumberOfTasks == ( UBaseType_t ) 1 )
			{
				/* 初始化任务相关的列表 */
                prvInitialiseTaskLists();
			}
		}
		else /* 如果pxCurrentTCB不为空，则根据任务的优先级将pxCurrentTCB指向最高优先级任务的TCB */
		{
				if( pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority )
				{
					pxCurrentTCB = pxNewTCB;
				}
		}
		uxTaskNumber++;
        
		/* 将任务添加到就绪列表 */
        prvAddTaskToReadyList( pxNewTCB );

	}
	/* 退出临界段 */
	taskEXIT_CRITICAL();
}

	
// main.c中定义的任务控制块
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;
extern TCB_t IdleTaskTCB;
void vApplicationGetIdleTaskMemory( TCB_t **ppxIdleTaskTCBBuffer, 
                                    StackType_t **ppxIdleTaskStackBuffer, 
                                    uint32_t *pulIdleTaskStackSize );


/* 启动调度器函数 */
void vTaskStartScheduler( void ){
	/*************创建空闲任务****************/
	TCB_t *pxIdleTaskTCBBuffer = NULL;  // 指向空闲任务的tcb
	StackType_t *pxIdleTaskStackBuffer = NULL;  // 指向空闲任务栈的栈顶地址
	uint32_t ulIdleTaskStackSize;
	
	// 获取空闲任务的内存
	vApplicationGetIdleTaskMemory(&pxIdleTaskTCBBuffer, 
	                              &pxIdleTaskStackBuffer,
	                              &ulIdleTaskStackSize);
	
	// 创建空闲任务
	xIdleTaskHandle = xTaskCreateStatic( (TaskFunction_t) prvIdleTask, 
	                                     (char *)"IDLE",
																				 (uint32_t)ulIdleTaskStackSize,
																			  (void *) NULL,
																					(UBaseType_t) tskIDLE_PRIORITY,
																					(StackType_t *)pxIdleTaskStackBuffer,
																				(TCB_t *) pxIdleTaskTCBBuffer);
  
  // 将空闲任务添加到任务就绪列表开头，默认优先级最低
  // vListInsertEnd (&(pxReadyTasksLists[0]),&(((TCB_t *)pxIdleTaskTCBBuffer)->xStateListItem));
		
	// 手动指定第一个运行的任务
	// pxCurrentTCB = &Task1TCB;
																				
	/* 初始化系统时基计数器 */
  xTickCount = ( TickType_t ) 0U;
	
	// 启动调度器
  if( xPortStartScheduler() != pdFALSE ){
		
	}

}

/* 空闲任务执行函数 */
static portTASK_FUNCTION( prvIdleTask, pvParameters ){
		/* 防止编译器的警告 */
	( void ) pvParameters;
    
    for(;;)
    {
        /* 空闲任务暂时什么都不做 */
    }
}


/* 任务切换函数，在PendSV中断服务函数中调用 */
#if 1
void vTaskSwitchContext(void){
	/* 查找最高优先级就绪任务的tcb，并更新到pxCurrentTCB */
	taskSELECT_HIGHEST_PRIORITY_TASK();
	
//	/* 暂不支持优先级，只进行两个任务的轮流切换 */
//	if(pxCurrentTCB == &Task1TCB){
//		pxCurrentTCB = &Task2TCB;
//	
//	}
//	else{
//		pxCurrentTCB = &Task1TCB;
//	}
	
}
#else
void vTaskSwitchContext(void){
	
	/* 若当前任务是空闲任务则尝试执行任务1或者2，
	看它们的延时是否结束，若都没结束则继续执行空闲任务	*/	
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
	若当前不是空闲任务，则检查另外一个任务，若另外一个任务不在延时中则切换到该任务，
	若在延时中，则判断当前任务是否在延时状态，是的话就切换到空闲任务，否则不切换
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

/* 阻塞延时函数 */
void vTaskDelay( const TickType_t xTicksToDelay ){
	TCB_t *pxTCB = NULL;
	
	/* 获取当前任务tcb */
	pxTCB = pxCurrentTCB;
	
	/* 设置延时时间 */
	pxTCB->xTicksToDelay = xTicksToDelay;
	
	/* 将任务从就绪列表中移除 */
	// uxListRemove(&(pxTCB->xStateListItem));
	taskRESET_READY_PRIORITY(pxTCB->uxPriority);
	
	
	/* 任务切换 */
	taskYIELD();
}

/* 更新系统时基 */
void xTaskIncrementTick(void){
	TCB_t *pxTCB = NULL;
  BaseType_t i = 0;
	
	/* 更新系统时基变量，时基加1 */
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;
	
	/* 扫描就绪列表中所有任务的xTicksToDelay，如果不为0，就减1 */
	for(i = 0;i < configMAX_PRIORITIES; i++){
		pxTCB = (TCB_t*) listGET_OWNER_OF_HEAD_ENTRY(( &pxReadyTasksLists[i] ));
		if(pxTCB->xTicksToDelay > 0){
			pxTCB->xTicksToDelay --;
			/* 延时时间到0，将位图表uxTopReadyPriority中对应位置置位 */
			if(pxTCB->xTicksToDelay == 0){
				taskRECORD_READY_PRIORITY(pxTCB->uxPriority);
			}
		}
	}
	
	/* 任务切换 */
	portYIELD();

}
