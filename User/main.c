/*!
* \file main.c
* \brief ��c�ļ�
* 
*��ϸ���� 
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


/* ����ջ���� */
TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE   128
StackType_t Task1Stack[TASK1_STACK_SIZE];

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE   128
StackType_t Task2Stack[TASK2_STACK_SIZE];

/* ����������ƿ� */
TCB_t Task1TCB;
TCB_t Task2TCB;


/* ���������� */
// �����ʱ����
void delay(uint32_t count){
	for(;count!=0;count--);

}

/* ����1 */

void Task1_Entry(void *p_arg){
	for(;;){
		flag1 = 1;
		delay(100);
		flag1 = 0;
		delay(100);
		
		/* �����л����ֶ��л� */
		taskYIELD();
	
	}
}

/* ����2 */

void Task2_Entry(void *p_arg){
	for(;;){
		flag2 = 1;
		delay(100);
		flag2 = 0;
		delay(100);
		
		/* �����л����ֶ��л� */
		taskYIELD();	
	
	}


}


int main(void){

	// ��ʼ�������б�
	prvInitialiseTaskLists();
	
	// ��������1
	Task1_Handle = xTaskCreateStatic((TaskFunction_t) Task1_Entry, // �������
	                                  (char *)"Job1",              // ��������
																			(uint32_t)TASK1_STACK_SIZE,  // ����ջ��С
																		(void*)NULL,                 // �������
																			(StackType_t *)Task1Stack,  // ����ջ��ʼ��ַ
																		(TCB_t*)&Task1TCB);           // ������ƿ�
  // ������1�ڵ���뵽��������б���
	vListInsertEnd(&(pxReadyTasksLists[1]),&(((TCB_t*)(&Task1TCB))->xStateListItem));
	
	Task2_Handle = xTaskCreateStatic( (TaskFunction_t)Task2_Entry,   /* ������� */
					                  (char *)"Task2",               /* �������ƣ��ַ�����ʽ */
					                  (uint32_t)TASK2_STACK_SIZE ,   /* ����ջ��С����λΪ�� */
					                  (void *) NULL,                 /* �����β� */
					                  (StackType_t *)Task2Stack,     /* ����ջ��ʼ��ַ */
					                  (TCB_t *)&Task2TCB );          /* ������ƿ� */
   /* ��������ӵ������б� */                                 
  vListInsertEnd( &( pxReadyTasksLists[2] ), &( ((TCB_t *)(&Task2TCB))->xStateListItem ) );																
	
  /* ��������������ʼ����������л� */
	vTaskStartScheduler();
	
	for(;;){
	
	}

}
