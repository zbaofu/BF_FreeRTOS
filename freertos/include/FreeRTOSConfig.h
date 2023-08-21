/*!
* \file FreeRTOSConfig.h
* \brief RTOS����ͷ�ļ�
* 
*��ϸ���� 
* 
* \author zbaofu
* \version V1.1
* \date 20230810 
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


#define configUSE_16_BIT_TICKS              0
#define configMAX_TASK_NAME_LEN             16  // �������Ƴ���
#define configSUPPORT_STATIC_ALLOCATION     1   //��̬���񴴽�
#define configMAX_PRIORITIES                5  // ��������б�����Ĵ�С,���֧��256�����ȼ�
#define configKERNEL_INTERRUPT_PRIORITY 		255   /* ����λ��Ч��������0xff��������15 */
#define configMINIMAL_STACK_SIZE         ((unsigned short) 128)  // ��С������ջ�ռ�


// ��RTOS���жϷ������������ú��ж�����ע���һ��
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler
#define vPortSVCHandler      SVC_Handler


#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* ����λ��Ч��������0xb0��������11 */

/* ϵͳʱ�Ӵ�С��CM3��25M���о���Ӳ����Ҫ���ó�Ӳ��ϵͳʱ�� */
#define configCPU_CLOCK_HZ			        ( ( unsigned long ) 25000000 )	 
/* SysTickÿ���ж϶��ٴΣ�Ŀǰ����100����ÿ1ms�ж�һ�� */
#define configTICK_RATE_HZ			        ( ( TickType_t ) 100 )

#endif
