/*!
* \file FreeRTOSConfig.h
* \brief RTOS配置头文件
* 
*详细概述 
* 
* \author zbaofu
* \version V1.1
* \date 20230810 
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


#define configUSE_16_BIT_TICKS              0
#define configMAX_TASK_NAME_LEN             16  // 任务名称长度
#define configSUPPORT_STATIC_ALLOCATION     1   //静态任务创建
#define configMAX_PRIORITIES                5  // 任务就绪列表数组的大小,最大支持256个优先级
#define configKERNEL_INTERRUPT_PRIORITY 		255   /* 高四位有效，即等于0xff，或者是15 */
#define configMINIMAL_STACK_SIZE         ((unsigned short) 128)  // 最小的任务栈空间


// 将RTOS的中断服务函数名称设置和中断向量注册表一致
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler
#define vPortSVCHandler      SVC_Handler


#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* 高四位有效，即等于0xb0，或者是11 */

/* 系统时钟大小，CM3是25M，有具体硬件需要设置成硬件系统时钟 */
#define configCPU_CLOCK_HZ			        ( ( unsigned long ) 25000000 )	 
/* SysTick每秒中断多少次，目前配置100，即每1ms中断一次 */
#define configTICK_RATE_HZ			        ( ( TickType_t ) 100 )

#endif
