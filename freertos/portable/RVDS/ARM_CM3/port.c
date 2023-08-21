/*!
* \file port.c
* \brief 接口文件
* 
*详细概述 
* 
* \author zbaofu
* \version V1.1
* \date 20230816 
*/


#include "FreeRTOS.h"
#include "task.h"
#include "ARMCM3.h"


/* 临界段嵌套计数器，在调度器启动时初始化为0 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;


#define portINITIAL_XPSR    (0x1000000)
#define portSTART_ADDRESS_MASK     ((StackType_t) 0xfffffffeUL)


/*
 * 参考stm32f103使用手册
 * 在Cortex-M中，内核外设SCB中SHPR3寄存器用于设置SysTick和PendSV的异常优先级
 * System handler priority register 3 (SCB_SHPR3) SCB_SHPR3：0xE000 ED20
 * Bits 31:24 PRI_15[7:0]: Priority of system handler 15, SysTick exception 
 * Bits 23:16 PRI_14[7:0]: Priority of system handler 14, PendSV 
 */
#define portNVIC_SYSPRI2_REG    (*((volatile uint32_t *) 0xe000ed20))
#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* SysTick控制寄存器，地址见权威指南 */
#define portNVIC_SYSTICK_CTRL_REG (*((volatile uint32_t *) 0xe000e010))
/* SysTick 重装载寄存器 */
#define portNVIC_SYSTICK_LOAD_REG (*((volatile uint32_t *) 0xe000e014))
/* SysTick时钟源选择 */
#ifndef configSYSTICK_CLOCK_HZ
  #define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
  // 确保SysTick的时钟和内核一致
	#define portNVIC_SYSTICK_CLK_BIT (1UL << 2UL) 
#else
	#define portNVIC_SYSTICK_CLK_BIT (0) 
#endif

#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )


/* 函数声明 */
void prvStartFirstTask( void );
void vPortSVCHandler( void );
void xPortPendSVHandler( void );
void vPortSetupTimerInterrupt( void );
void xPortSysTickHandler(void);


static void prvTaskExitError(void){
	// 函数停止
	for(;;);
	
}

/************** 初始化任务栈，更新栈顶指针，存储任务运行的环境参数 ***********/
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack, // 栈顶地址
																	 TaskFunction_t pxCode,   // 任务入口
                                    void *pvParameters)   // 任务形参
{
 /*
     异常发生时，CPU自动从栈内加载到CPU寄存器的内容，包括8个寄存器，R0、R1
     R2、R3、R12、R14、R15和xPSR的位24，xPSR的第24位必须为1，设置Thumb状态
 */
	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_XPSR;
	pxTopOfStack--;
	*pxTopOfStack = ((StackType_t) pxCode ) & portSTART_ADDRESS_MASK;
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t) prvTaskExitError;
	
	/* R12，R3，R2和R1默认初始化为0 */
	pxTopOfStack -= 5;
	*pxTopOfStack = (StackType_t) pvParameters;
	
	/* 异常发生时，需要手动加载到CPU寄存器的内容，默认初始化为0 */
	pxTopOfStack -=8;
	
	/* 返回栈顶指针，此时pxTopOfStack指向空闲栈 */
	return pxTopOfStack;																	
																		
}			

/**************** 启动调度器，运行第一个任务*******************/
BaseType_t xPortStartScheduler(void){ 
	/* 配置PendSV和SysTick的中断优先级为最低 */
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;
	
	/* 初始化SysTick */
	vPortSetupTimerInterrupt();
	
	/* 启动第一个任务 */
	prvStartFirstTask();
	
	return 0;

}

/*******************  运行第一个任务 ***********************/
/*
 * 参考资料《STM32F10xxx Cortex-M3 programming manual》4.4.3
 * 在Cortex-M中，内核外设SCB的地址范围为：0xE000ED00-0xE000ED3F
 * Cortex-M系统控制块(SCB)是内核外设的主要模块之一，提供系统控制以及系统执行信息，包括配置，控制，报告系统异常等
 * 0xE000ED08为SCB外设中SCB_VTOR这个寄存器的地址，里面存放的是向量表的起始地址，即MSP的地址
 * 更新MSP值，产生SVC系统调用，然后去SVC的中断服务函数中切换到第一个任务
 */

__asm void prvStartFirstTask(void)
{

	/* 当前栈地址按照8字节对齐 */
	PRESERVE8
	
	
	/* 在Cortex-M中，0xE000ED08是SCB_VTOR寄存器的地址,里面存放的是向量表的起始地址，即MSP地址
	   向量表通常从内部FALSH起始地址开始存放，可知memory：0x00000000存放的MSP的值
	*/	
	ldr r0, = 0xE000ED08   // 将0xE000ED08立即数加载到寄存器R0
	ldr r0, [r0]      // 将0xE000ED08地址指向的内容加载到寄存器R0，这是R0就等于SCB_VTOR寄存器的值0x00000000
	ldr r0, [r0]      // 将0x00000000地址指向的内容加载到R0，R0就等于0x200008DB
	
	/* 设置主堆栈指针MSP的值 */
	msr msp, r0  // 将R0的值存储到MSP，MSP的值就等于0x200008DB，也就是主堆栈的栈顶指针
	
	/* 使能全局中断 */
	cpsie i    // 开中断
	cpsie f     // 开异常
	
	dsb  // 数据隔离
	isb  // 指令隔离
	
	/* 调用SVC启动第一个任务 */
  svc 0
	nop
	nop
}


/********************** SVC(系统服务调用)中断服务函数 *************/
__asm void vPortSVCHandler(void){
	// 当前正在运行的任务的任务控制块
	extern pxCurrentTCB;
	
	PRESERVE8
	
	ldr r3, = pxCurrentTCB  // 加载pxCurrentTCB的地址到r3
	ldr r1, [r3]     // 加载pxCurrentTCB到r1
	/* 
	加载pxCurrentTCB指向的任务控制块到r0，
	任务控制块第一个成员是栈顶指针，此时r0就等于栈顶指针pxTopOfStack
	*/
	ldr r0, [r1]    
	ldmia r0!, {r4-r11}  // 以r0为基地址，将栈中向上增长的8个字的内容加载到CPU寄存器r4-r11,r0也递增
	msr psp, r0   // 将新的栈顶指针r0更新到psp，psp是任务执行时候使用的堆栈指针
	
	isb   // 指令隔离
	mov r0, #0  // r0寄存器清零
	msr basepri, r0 // 设置basepro寄存器的为0，即打开所有中断，0是缺省值
	/*
	设置SVC中断服务使用PSP而不是MSP堆栈指针
	当r14为0xFFFFFFFX，执行中断返回指令，其中X的0位为1表示返回thumb状态，
	1位和2位决定返回是用MSP还是PSP，以及是特权模式还是用户模式
	*/
	orr r14, #0xd  // 将r14寄存器最后4位按位或上0x0d
	
	/* 
	异常返回，出栈使用的是PSP指针，
	自动将栈中剩下的内容加载到CPU寄存器，xPSR，PC（任务入口地址），R14，R12，R3，R2，R1，R0 （任务的形参）
	同时PSP的值也将更新，即指向任务栈的栈顶
  */
	bx r14  // 返回 r14是lr寄存器，保存异常返回标志，包括返回后进入任务模式还是处理器模式、使用 PSP堆栈指针还是 MSP堆栈指针。
	
}

/******************** PendSV中断服务函数，实现任务切换 *******************/
__asm void xPortPendSVHandler(void){
	extern pxCurrentTCB;   // 指向当前运行任务的tcb
	extern vTaskSwitchContext;  // 指向任务切换函数

	PRESERVE8
	
	/*
	将psp的值存储到r0，当进入到此函数时，上一个任务的环境会自动存储到当前任务的任务栈内
	剩下的r4-r11需要手动保存，同时psp会自动更新
	*/
	mrs r0, psp   
	isb           // 指令隔离
	 
	ldr r3, = pxCurrentTCB   // 加载pxCurrentTCB地址到r3
	ldr r2, [r3]            // 加载r3指向的内容到r2，即r2等于pxCurrentTCB
	
	stmdb r0!, {r4-r11}     // 以r0为基址，将CPU寄存器r4-r11存储到任务栈，指针先递减，再操作
	/* 
	将r0的值存储到r2指向的内容，根据任务栈结构和上面的操作，这时r0指向pxTopOfStack，r2等于pxCurrentTCB，
	因此就是把栈顶指针存到原来的pxCurrentTCB，完成上下文切换的上文保存	
	*/
	str r0, [r2]   
	
	/*
	将r3和r14临时压入堆栈，r3存储pxCurrentTCB的地址，r14是LR寄存器，存储返回地址。
	接下来要调用函数vTaskSwitchContext，返回地址会自动保存到r14，会覆盖当前r14的值，
	而中断服务函数执行完，返回时要根据r14的值来决定返回处理器还是任务模式，
	出栈时用MSP还是PSP，因此要把当前r14进行入栈保护
	同样函数调用后r3也会被更新，因此同样入栈保护	
	*/
	stmdb sp!, {r3, r14}
	
	/*
	将configMAX_SYSCALL_INTERRUPT_PRIORITY的值赋给r0，用来配置中断屏蔽寄存器basepri，高四位有效
	目前配置为191，1011111，则实际值为11，即优先级高于或等于11的中断被屏蔽，数字越大，优先级越低
	*/
	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0  // 关中断，进入临界段
	dsb
	isb
	
	
	bl vTaskSwitchContext  // 调用任务切换函数，切换pxCurrentTCB，指向下一任务的tcb
	mov r0, #0
	msr basepri, r0       // 开中断，退出临界段
	ldmia sp!, {r3,r14}   // 从栈中取出r3和r14
	
	ldr r1, [r3]  // 加载r3指向的内容到r1，即让r1等于pxCurrentTCB，指向下一个要运行的任务的tcb
	ldr r0, [r1]  // 加载r1指向的内容到r0，即加载任务栈顶指针到r0
	ldmia r0!, {r4-r11}  // 以r0为基地址，将下一个要运行的栈中向上增长的8个字的内容加载到CPU寄存器r4-r11,r0也递增
	msr psp, r0   // 更新psp的值
	isb
	
	/*
	异常返回，根据上文入栈操作和上一函数的设置，r14等于0xFFFFFFFd表示返回后进入任务模式、以psp作为堆栈指针，
	出栈后psp指向栈顶，同时将任务栈内剩余的内容加载到CPU寄存器
	*/
	bx r14   
	nop
	
}

/**************** 不带中断保护的进入临界段 **********************/
void vPortEnterCritical(void){
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;
	if(uxCriticalNesting == 1){	
		/*
			若uxCriticalNesting等于1，即一层嵌套，要确保当前没有中断活跃，
		  即中断控制寄存器SCB_ICSR低8位要等于0
		*/
		// configASSERT((portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) == 0);
	}
}


/********************* 不带中断保护退出临界段 *************************/
void vPortExitCritical(){
	//configASSERT( uxCriticalNesting );
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
	}
}


/************************* SysTick中断服务函数 **************************/
void xPortSysTickHandler(void){
	// 进入临界段，关中断
	vPortRaiseBASEPRI();
	
	// 更新系统时基
	xTaskIncrementTick();
	
  // 退出临界段，开中断
	vPortClearBASEPRIFromISR();
}

/******************************初始化SysTick*****************************/
void vPortSetupTimerInterrupt( void ){
	/* 设置重装载寄存器的值 */
	portNVIC_SYSTICK_LOAD_REG = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
	/*
	设置SysTick控制寄存器
	设置系统定时器的时钟等于内核时钟
	使能SysTick定时器中断
	使能SysTick定时器
	*/
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}

