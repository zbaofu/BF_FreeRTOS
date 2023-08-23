# BF_RTOS
## 目标
实现Free_RTOS内核  

## 描述
FreeRTOS内核源码学习记录，按照《野火Free_RTOS内核实现于应用开发实战指南》一书进行Free_RTOS内核开发。
在stm32f103单片机上运行，内核是ARMv7架构的Cortex-M3,编译环境Keilv5，按照实战指南的说明进行开发。  
过程中发现指南中未对一些汇编指令和CM3寄存器设置做出详细说明，因此增加了详细的程序注释和说明，详见项目日志。  

## 进展
实现了任务创建和切换、临界段保护、空闲任务和阻塞延时、链表的一些操作、支持任务多优先级

## 项目日志
https://vintage-march-2cf.notion.site/BF_RTOS-0272bf91d4c34ba2bae2bbe89312ba36?pvs=4

## 参考资料
《野火FreeRTOS内核实现与应用开发实战指南》  
《Cortex-M3权威指南》（中文）  
