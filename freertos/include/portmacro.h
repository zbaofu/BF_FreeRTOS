/*!
* \file portmacro.h
* \brief 将标准的C语言的数据类型取新的类型名
* 
*详细概述 
* 
* \author zbaofu
* \version V1.1
* \date 20230810 
*/

#ifndef PORTMACRO_H
#define PORTMACRO_H


#include "stdint.h"
#include "stddef.h"
#include "FreeRTOSConfig.h"


// 数据类型重定义
#define portCHAR char
#define portFLOAT float
#define portDOUBLE double
#define portLONG long
#define portSHORT short
#define portSTACK_TYPE uint32_t
#define portBASE_TYPE long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if ( configUSE_16_BIT_TICKs == 1 )
typedef uint16_t TickType_t;
#define portMAX_DELAY (TickType_t) 0xffff
#else
typedef uint32_t TickType_t;
#define portMAX_DELAY (TickType_t) 0xffffffffUL
#endif

/*
  中断控制状态寄存器：0xe000ed04
  Bit 28 PENDSVSET: PendSV 悬起位
*/
#define portNVIC_INT_CTRL_REG   (*((volatile uint32_t *)0xe000ed04))
#define portNVIC_PENDSVSET_BIT  (1UL << 28UL)

#define portSY_FULL_READ_WRITE  (15)

/*
portYIELD的实现就是将 PendSV的悬起位置 1，当没有其它中断运行的时候响应 PendSV中断，
去执行我们写好的 PendSV中断服务函数，在 里面实现任务切换。
*/
#define portYIELD()                               \
{                                                 \
	/* 触发PendSV，产生上下文切换 */                  \
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
	__dsb( portSY_FULL_READ_WRITE );                \
	__isb( portSY_FULL_READ_WRITE );                \
}

#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
	
#ifndef portFORCE_INLINE
	#define portFORCE_INLINE __forceinline
#endif

/************** 多优先级配置 *****************/
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1  // 查找最高优先级的就绪任务方法选择
#endif
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* 检查最大优先级是是否超过32 */
	#if( configMAX_PRIORITIES > 32 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif
		
	/* 根据优先级 设置/清除优先级位中相应的位,
		将uxReadyPriorities变成位图，对相应的优先级位进行置位和清除，用于使用clz函数 */
	#define portRECORD_READY_PRIORITY(uxPriority, uxReadyPriorities) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )
	
	/* 查找最高优先级，通过计算uxReadyPriorities前异零clz，第一次出现1的位前面零的个数 */
	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )


#endif



/********* 临界区管理，进行中断的开关 ***********/

// 设置内联inline修饰符接口
#define portINLINE __inline

/* 非中断保护的进出临界段的宏 */
#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()

/* 不带中断保护的开关中断函数 */
#define portDISABLE_INTERRUPTS() vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS() vPortSetBASEPRI( 0 ) /* 函数参数设置为0，不带中断保护 */

/* 带中断保护的开关中断函数 */
#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortSetBASEPRI(x) /* 函数参数设置成关中断时寄存器的值*/


/* 不带返回值的关中断函数，无法嵌套，不能在中断中使用 */
static portFORCE_INLINE void vPortRaiseBASEPRI( void )
{
uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* 关中断，将basepri寄存器设置为系统所允许的最大的优先级号，
		其他中断的优先级也就高于这个最大值，优先级号越大，优先级越小。 */
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}
}

/* 带返回值的关中断函数，可以嵌套，可以在中断里面使用 */
static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI( void )
{
uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* 与上文一样，关中断，同时把basepri寄存器的值保存并返回 */
		mrs ulReturn, basepri
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}

	return ulReturn;
}

/* 开中断，降低basepri的值  */
static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEPRI ){
	__asm
	{
		// 开中断
		msr basepri, ulBASEPRI 
	
	}
}

/* 开中断，直接将basepri寄存器的值设为0 */
static portFORCE_INLINE void vPortClearBASEPRIFromISR( void )
{
	__asm
	{
		/* Set BASEPRI to 0 so no interrupts are masked.  This function is only
		used to lower the mask in an interrupt, so memory barriers are not 
		used. */
		msr basepri, #0
	}
}



#endif
