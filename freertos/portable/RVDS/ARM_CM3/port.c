/*!
* \file port.c
* \brief �ӿ��ļ�
* 
*��ϸ���� 
* 
* \author zbaofu
* \version V1.1
* \date 20230816 
*/


#include "FreeRTOS.h"
#include "task.h"
#include "ARMCM3.h"


/* �ٽ��Ƕ�׼��������ڵ���������ʱ��ʼ��Ϊ0 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;


#define portINITIAL_XPSR    (0x1000000)
#define portSTART_ADDRESS_MASK     ((StackType_t) 0xfffffffeUL)


/*
 * �ο�stm32f103ʹ���ֲ�
 * ��Cortex-M�У��ں�����SCB��SHPR3�Ĵ�����������SysTick��PendSV���쳣���ȼ�
 * System handler priority register 3 (SCB_SHPR3) SCB_SHPR3��0xE000 ED20
 * Bits 31:24 PRI_15[7:0]: Priority of system handler 15, SysTick exception 
 * Bits 23:16 PRI_14[7:0]: Priority of system handler 14, PendSV 
 */
#define portNVIC_SYSPRI2_REG    (*((volatile uint32_t *) 0xe000ed20))
#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* SysTick���ƼĴ�������ַ��Ȩ��ָ�� */
#define portNVIC_SYSTICK_CTRL_REG (*((volatile uint32_t *) 0xe000e010))
/* SysTick ��װ�ؼĴ��� */
#define portNVIC_SYSTICK_LOAD_REG (*((volatile uint32_t *) 0xe000e014))
/* SysTickʱ��Դѡ�� */
#ifndef configSYSTICK_CLOCK_HZ
  #define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
  // ȷ��SysTick��ʱ�Ӻ��ں�һ��
	#define portNVIC_SYSTICK_CLK_BIT (1UL << 2UL) 
#else
	#define portNVIC_SYSTICK_CLK_BIT (0) 
#endif

#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )


/* �������� */
void prvStartFirstTask( void );
void vPortSVCHandler( void );
void xPortPendSVHandler( void );
void vPortSetupTimerInterrupt( void );
void xPortSysTickHandler(void);


static void prvTaskExitError(void){
	// ����ֹͣ
	for(;;);
	
}

/************** ��ʼ������ջ������ջ��ָ�룬�洢�������еĻ������� ***********/
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack, // ջ����ַ
																	 TaskFunction_t pxCode,   // �������
                                    void *pvParameters)   // �����β�
{
 /*
     �쳣����ʱ��CPU�Զ���ջ�ڼ��ص�CPU�Ĵ��������ݣ�����8���Ĵ�����R0��R1
     R2��R3��R12��R14��R15��xPSR��λ24��xPSR�ĵ�24λ����Ϊ1������Thumb״̬
 */
	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_XPSR;
	pxTopOfStack--;
	*pxTopOfStack = ((StackType_t) pxCode ) & portSTART_ADDRESS_MASK;
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t) prvTaskExitError;
	
	/* R12��R3��R2��R1Ĭ�ϳ�ʼ��Ϊ0 */
	pxTopOfStack -= 5;
	*pxTopOfStack = (StackType_t) pvParameters;
	
	/* �쳣����ʱ����Ҫ�ֶ����ص�CPU�Ĵ��������ݣ�Ĭ�ϳ�ʼ��Ϊ0 */
	pxTopOfStack -=8;
	
	/* ����ջ��ָ�룬��ʱpxTopOfStackָ�����ջ */
	return pxTopOfStack;																	
																		
}			

/**************** ���������������е�һ������*******************/
BaseType_t xPortStartScheduler(void){ 
	/* ����PendSV��SysTick���ж����ȼ�Ϊ��� */
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;
	
	/* ��ʼ��SysTick */
	vPortSetupTimerInterrupt();
	
	/* ������һ������ */
	prvStartFirstTask();
	
	return 0;

}

/*******************  ���е�һ������ ***********************/
/*
 * �ο����ϡ�STM32F10xxx Cortex-M3 programming manual��4.4.3
 * ��Cortex-M�У��ں�����SCB�ĵ�ַ��ΧΪ��0xE000ED00-0xE000ED3F
 * Cortex-Mϵͳ���ƿ�(SCB)���ں��������Ҫģ��֮һ���ṩϵͳ�����Լ�ϵͳִ����Ϣ���������ã����ƣ�����ϵͳ�쳣��
 * 0xE000ED08ΪSCB������SCB_VTOR����Ĵ����ĵ�ַ�������ŵ������������ʼ��ַ����MSP�ĵ�ַ
 * ����MSPֵ������SVCϵͳ���ã�Ȼ��ȥSVC���жϷ��������л�����һ������
 */

__asm void prvStartFirstTask(void)
{

	/* ��ǰջ��ַ����8�ֽڶ��� */
	PRESERVE8
	
	
	/* ��Cortex-M�У�0xE000ED08��SCB_VTOR�Ĵ����ĵ�ַ,�����ŵ������������ʼ��ַ����MSP��ַ
	   ������ͨ�����ڲ�FALSH��ʼ��ַ��ʼ��ţ���֪memory��0x00000000��ŵ�MSP��ֵ
	*/	
	ldr r0, = 0xE000ED08   // ��0xE000ED08���������ص��Ĵ���R0
	ldr r0, [r0]      // ��0xE000ED08��ַָ������ݼ��ص��Ĵ���R0������R0�͵���SCB_VTOR�Ĵ�����ֵ0x00000000
	ldr r0, [r0]      // ��0x00000000��ַָ������ݼ��ص�R0��R0�͵���0x200008DB
	
	/* ��������ջָ��MSP��ֵ */
	msr msp, r0  // ��R0��ֵ�洢��MSP��MSP��ֵ�͵���0x200008DB��Ҳ��������ջ��ջ��ָ��
	
	/* ʹ��ȫ���ж� */
	cpsie i    // ���ж�
	cpsie f     // ���쳣
	
	dsb  // ���ݸ���
	isb  // ָ�����
	
	/* ����SVC������һ������ */
  svc 0
	nop
	nop
}


/********************** SVC(ϵͳ�������)�жϷ����� *************/
__asm void vPortSVCHandler(void){
	// ��ǰ�������е������������ƿ�
	extern pxCurrentTCB;
	
	PRESERVE8
	
	ldr r3, = pxCurrentTCB  // ����pxCurrentTCB�ĵ�ַ��r3
	ldr r1, [r3]     // ����pxCurrentTCB��r1
	/* 
	����pxCurrentTCBָ���������ƿ鵽r0��
	������ƿ��һ����Ա��ջ��ָ�룬��ʱr0�͵���ջ��ָ��pxTopOfStack
	*/
	ldr r0, [r1]    
	ldmia r0!, {r4-r11}  // ��r0Ϊ����ַ����ջ������������8���ֵ����ݼ��ص�CPU�Ĵ���r4-r11,r0Ҳ����
	msr psp, r0   // ���µ�ջ��ָ��r0���µ�psp��psp������ִ��ʱ��ʹ�õĶ�ջָ��
	
	isb   // ָ�����
	mov r0, #0  // r0�Ĵ�������
	msr basepri, r0 // ����basepro�Ĵ�����Ϊ0�����������жϣ�0��ȱʡֵ
	/*
	����SVC�жϷ���ʹ��PSP������MSP��ջָ��
	��r14Ϊ0xFFFFFFFX��ִ���жϷ���ָ�����X��0λΪ1��ʾ����thumb״̬��
	1λ��2λ������������MSP����PSP���Լ�����Ȩģʽ�����û�ģʽ
	*/
	orr r14, #0xd  // ��r14�Ĵ������4λ��λ����0x0d
	
	/* 
	�쳣���أ���ջʹ�õ���PSPָ�룬
	�Զ���ջ��ʣ�µ����ݼ��ص�CPU�Ĵ�����xPSR��PC��������ڵ�ַ����R14��R12��R3��R2��R1��R0 ��������βΣ�
	ͬʱPSP��ֵҲ�����£���ָ������ջ��ջ��
  */
	bx r14  // ���� r14��lr�Ĵ����������쳣���ر�־���������غ��������ģʽ���Ǵ�����ģʽ��ʹ�� PSP��ջָ�뻹�� MSP��ջָ�롣
	
}

/******************** PendSV�жϷ�������ʵ�������л� *******************/
__asm void xPortPendSVHandler(void){
	extern pxCurrentTCB;   // ָ��ǰ���������tcb
	extern vTaskSwitchContext;  // ָ�������л�����

	PRESERVE8
	
	/*
	��psp��ֵ�洢��r0�������뵽�˺���ʱ����һ������Ļ������Զ��洢����ǰ���������ջ��
	ʣ�µ�r4-r11��Ҫ�ֶ����棬ͬʱpsp���Զ�����
	*/
	mrs r0, psp   
	isb           // ָ�����
	 
	ldr r3, = pxCurrentTCB   // ����pxCurrentTCB��ַ��r3
	ldr r2, [r3]            // ����r3ָ������ݵ�r2����r2����pxCurrentTCB
	
	stmdb r0!, {r4-r11}     // ��r0Ϊ��ַ����CPU�Ĵ���r4-r11�洢������ջ��ָ���ȵݼ����ٲ���
	/* 
	��r0��ֵ�洢��r2ָ������ݣ���������ջ�ṹ������Ĳ�������ʱr0ָ��pxTopOfStack��r2����pxCurrentTCB��
	��˾��ǰ�ջ��ָ��浽ԭ����pxCurrentTCB������������л������ı���	
	*/
	str r0, [r2]   
	
	/*
	��r3��r14��ʱѹ���ջ��r3�洢pxCurrentTCB�ĵ�ַ��r14��LR�Ĵ������洢���ص�ַ��
	������Ҫ���ú���vTaskSwitchContext�����ص�ַ���Զ����浽r14���Ḳ�ǵ�ǰr14��ֵ��
	���жϷ�����ִ���꣬����ʱҪ����r14��ֵ���������ش�������������ģʽ��
	��ջʱ��MSP����PSP�����Ҫ�ѵ�ǰr14������ջ����
	ͬ���������ú�r3Ҳ�ᱻ���£����ͬ����ջ����	
	*/
	stmdb sp!, {r3, r14}
	
	/*
	��configMAX_SYSCALL_INTERRUPT_PRIORITY��ֵ����r0�����������ж����μĴ���basepri������λ��Ч
	Ŀǰ����Ϊ191��1011111����ʵ��ֵΪ11�������ȼ����ڻ����11���жϱ����Σ�����Խ�����ȼ�Խ��
	*/
	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0  // ���жϣ������ٽ��
	dsb
	isb
	
	
	bl vTaskSwitchContext  // ���������л��������л�pxCurrentTCB��ָ����һ�����tcb
	mov r0, #0
	msr basepri, r0       // ���жϣ��˳��ٽ��
	ldmia sp!, {r3,r14}   // ��ջ��ȡ��r3��r14
	
	ldr r1, [r3]  // ����r3ָ������ݵ�r1������r1����pxCurrentTCB��ָ����һ��Ҫ���е������tcb
	ldr r0, [r1]  // ����r1ָ������ݵ�r0������������ջ��ָ�뵽r0
	ldmia r0!, {r4-r11}  // ��r0Ϊ����ַ������һ��Ҫ���е�ջ������������8���ֵ����ݼ��ص�CPU�Ĵ���r4-r11,r0Ҳ����
	msr psp, r0   // ����psp��ֵ
	isb
	
	/*
	�쳣���أ�����������ջ��������һ���������ã�r14����0xFFFFFFFd��ʾ���غ��������ģʽ����psp��Ϊ��ջָ�룬
	��ջ��pspָ��ջ����ͬʱ������ջ��ʣ������ݼ��ص�CPU�Ĵ���
	*/
	bx r14   
	nop
	
}

/**************** �����жϱ����Ľ����ٽ�� **********************/
void vPortEnterCritical(void){
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;
	if(uxCriticalNesting == 1){	
		/*
			��uxCriticalNesting����1����һ��Ƕ�ף�Ҫȷ����ǰû���жϻ�Ծ��
		  ���жϿ��ƼĴ���SCB_ICSR��8λҪ����0
		*/
		// configASSERT((portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) == 0);
	}
}


/********************* �����жϱ����˳��ٽ�� *************************/
void vPortExitCritical(){
	//configASSERT( uxCriticalNesting );
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
	}
}


/************************* SysTick�жϷ����� **************************/
void xPortSysTickHandler(void){
	// �����ٽ�Σ����ж�
	vPortRaiseBASEPRI();
	
	// ����ϵͳʱ��
	xTaskIncrementTick();
	
  // �˳��ٽ�Σ����ж�
	vPortClearBASEPRIFromISR();
}

/******************************��ʼ��SysTick*****************************/
void vPortSetupTimerInterrupt( void ){
	/* ������װ�ؼĴ�����ֵ */
	portNVIC_SYSTICK_LOAD_REG = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
	/*
	����SysTick���ƼĴ���
	����ϵͳ��ʱ����ʱ�ӵ����ں�ʱ��
	ʹ��SysTick��ʱ���ж�
	ʹ��SysTick��ʱ��
	*/
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}

