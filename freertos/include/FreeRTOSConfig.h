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


#define configUSE_16_BIT_TICKS   0
#define configMAX_TASK_NAME_LEN  16  // �������Ƴ���
#define configSUPPORT_STATIC_ALLOCATION 1 //��̬���񴴽�
#define configMAX_PRIORITIES   5  // ��������б�����Ĵ�С,���֧��256�����ȼ�
#define configKERNEL_INTERRUPT_PRIORITY 		255   /* ����λ��Ч��������0xff��������15 */


// ��RTOS���жϷ������������ú��ж�����ע���һ��
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHabdler  SysTick_Handler
#define vPortSVCHandler      SVC_Handler


#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191   /* ����λ��Ч��������0xb0��������11 */










#endif
