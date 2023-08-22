/*!
* \file portmacro.h
* \brief ����׼��C���Ե���������ȡ�µ�������
* 
*��ϸ���� 
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


// ���������ض���
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
  �жϿ���״̬�Ĵ�����0xe000ed04
  Bit 28 PENDSVSET: PendSV ����λ
*/
#define portNVIC_INT_CTRL_REG   (*((volatile uint32_t *)0xe000ed04))
#define portNVIC_PENDSVSET_BIT  (1UL << 28UL)

#define portSY_FULL_READ_WRITE  (15)

/*
portYIELD��ʵ�־��ǽ� PendSV������λ�� 1����û�������ж����е�ʱ����Ӧ PendSV�жϣ�
ȥִ������д�õ� PendSV�жϷ��������� ����ʵ�������л���
*/
#define portYIELD()                               \
{                                                 \
	/* ����PendSV�������������л� */                  \
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
	__dsb( portSY_FULL_READ_WRITE );                \
	__isb( portSY_FULL_READ_WRITE );                \
}

#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
	
#ifndef portFORCE_INLINE
	#define portFORCE_INLINE __forceinline
#endif

/************** �����ȼ����� *****************/
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1  // ����������ȼ��ľ������񷽷�ѡ��
#endif
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* ���������ȼ����Ƿ񳬹�32 */
	#if( configMAX_PRIORITIES > 32 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif
		
	/* �������ȼ� ����/������ȼ�λ����Ӧ��λ,
		��uxReadyPriorities���λͼ������Ӧ�����ȼ�λ������λ�����������ʹ��clz���� */
	#define portRECORD_READY_PRIORITY(uxPriority, uxReadyPriorities) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )
	
	/* ����������ȼ���ͨ������uxReadyPrioritiesǰ����clz����һ�γ���1��λǰ����ĸ��� */
	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )


#endif



/********* �ٽ������������жϵĿ��� ***********/

// ��������inline���η��ӿ�
#define portINLINE __inline

/* ���жϱ����Ľ����ٽ�εĺ� */
#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()

/* �����жϱ����Ŀ����жϺ��� */
#define portDISABLE_INTERRUPTS() vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS() vPortSetBASEPRI( 0 ) /* ������������Ϊ0�������жϱ��� */

/* ���жϱ����Ŀ����жϺ��� */
#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortSetBASEPRI(x) /* �����������óɹ��ж�ʱ�Ĵ�����ֵ*/


/* ��������ֵ�Ĺ��жϺ������޷�Ƕ�ף��������ж���ʹ�� */
static portFORCE_INLINE void vPortRaiseBASEPRI( void )
{
uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* ���жϣ���basepri�Ĵ�������Ϊϵͳ��������������ȼ��ţ�
		�����жϵ����ȼ�Ҳ�͸���������ֵ�����ȼ���Խ�����ȼ�ԽС�� */
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}
}

/* ������ֵ�Ĺ��жϺ���������Ƕ�ף��������ж�����ʹ�� */
static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI( void )
{
uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* ������һ�������жϣ�ͬʱ��basepri�Ĵ�����ֵ���沢���� */
		mrs ulReturn, basepri
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}

	return ulReturn;
}

/* ���жϣ�����basepri��ֵ  */
static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEPRI ){
	__asm
	{
		// ���ж�
		msr basepri, ulBASEPRI 
	
	}
}

/* ���жϣ�ֱ�ӽ�basepri�Ĵ�����ֵ��Ϊ0 */
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
