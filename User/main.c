/*!
* \file main.c
* \brief 主c文件
* 
*详细概述 
* 
* \author zbaofu
* \version V1.1
* \date 20230811 
*/


#include "FreeRTOS.h"
#include "task.h"


extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
portCHAR flag1;
portCHAR flag2;


/* 任务栈定义 */
TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE   128
StackType_t Task1Stack[TASK1_STACK_SIZE];

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE   128
StackType_t Task2Stack[TASK2_STACK_SIZE];

/* 定义任务控制块 */
TCB_t Task1TCB;
TCB_t Task2TCB;


/* 定义任务函数 */
// 软件延时函数
void delay(uint32_t count){
	for(;count!=0;count--);

}

/* 任务1 */

void Task1_Entry(void *p_arg){
	for(;;){
		flag1 = 1;
		delay(100);
		flag1 = 0;
		delay(100);
		
		/* 任务切换，手动切换 */
		taskYIELD();
	
	}
}

/* 任务2 */

void Task2_Entry(void *p_arg){
	for(;;){
		flag2 = 1;
		delay(100);
		flag2 = 0;
		delay(100);
		
		/* 任务切换，手动切换 */
		taskYIELD();	
	
	}


}


int main(void){

	// 初始化就绪列表
	prvInitialiseTaskLists();
	
	// 创建任务1
	Task1_Handle = xTaskCreateStatic((TaskFunction_t) Task1_Entry, // 任务入口
	                                  (char *)"Job1",              // 任务名称
																			(uint32_t)TASK1_STACK_SIZE,  // 任务栈大小
																		(void*)NULL,                 // 任务参数
																			(StackType_t *)Task1Stack,  // 任务栈起始地址
																		(TCB_t*)&Task1TCB);           // 任务控制块
  // 将任务1节点插入到任务就绪列表中
	vListInsertEnd(&(pxReadyTasksLists[1]),&(((TCB_t*)(&Task1TCB))->xStateListItem));
	
	Task2_Handle = xTaskCreateStatic( (TaskFunction_t)Task2_Entry,   /* 任务入口 */
					                  (char *)"Task2",               /* 任务名称，字符串形式 */
					                  (uint32_t)TASK2_STACK_SIZE ,   /* 任务栈大小，单位为字 */
					                  (void *) NULL,                 /* 任务形参 */
					                  (StackType_t *)Task2Stack,     /* 任务栈起始地址 */
					                  (TCB_t *)&Task2TCB );          /* 任务控制块 */
   /* 将任务添加到就绪列表 */                                 
  vListInsertEnd( &( pxReadyTasksLists[2] ), &( ((TCB_t *)(&Task2TCB))->xStateListItem ) );																
	
  /* 启动调度器，开始多任务调度切换 */
	vTaskStartScheduler();
	
	for(;;){
	
	}

}
