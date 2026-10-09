/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK			0
/* 开启 tick hook：在 vApplicationTickHook() 中调用 HAL_IncTick()，
 * 把 HAL 时基挂到 FreeRTOS tick 上（见 BSP/app.c）。
 * 原因：SysTick_Handler 已交由 FreeRTOS port.c 接管，
 *       原 stm32f1xx_it.c 里的 HAL_IncTick() 被移除，
 *       若不在此处补回，HAL_GetTick() 将恒为 0，main 循环节拍判断失效。 */
#define configUSE_TICK_HOOK			1
#define configCPU_CLOCK_HZ			( ( unsigned long ) 72000000 )	
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES		( 5 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 10* 1024 ) )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY	0
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configUSE_MUTEXES			1
#define configUSE_RECURSIVE_MUTEXES	1
#define configUSE_COUNTING_SEMAPHORES 1
#define configQUEUE_REGISTRY_SIZE	8
#define configCHECK_FOR_STACK_OVERFLOW	2
#define configUSE_MALLOC_FAILED_HOOK	1
#define configUSE_APPLICATION_TASK_TAG	0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xb0, or priority 11. */


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

/*-----------------------------------------------------------
 * Cortex-M3 内核相关（野火模板缺失，必须补充）
 *----------------------------------------------------------*/

/* STM32F1 的 NVIC 优先级寄存器只实现高 4 位，
 * 此值必须与 stm32f1xx_hal_conf.h 中的 __NVIC_PRIO_BITS 保持一致（均为 4）。
 * 不一致会在 vPortValidateInterruptPriority() 中触发 configASSERT，
 * 表现为上电后静默卡死，无任何编译/链接错误。 */
#define configPRIO_BITS				4

/*-----------------------------------------------------------
 * 中断向量表钩子名称映射（本版本 FreeRTOS 必需）
 *----------------------------------------------------------
 * port.c 中定义的函数名为带前缀的 vPortSVCHandler /
 * xPortPendSVHandler / xPortSysTickHandler，而启动文件
 * (startup_stm32f103xb.s) 的向量表引用的是 SVC_Handler /
 * PendSV_Handler / SysTick_Handler，两者名字不同，无法自动接管。
 *
 * 通过以下宏做名称替换：编译 port.c 时名字被替换为无前缀版本，
 * 从而以强符号定义覆盖启动文件中的 [WEAK] 空壳实现。
 *
 * 若缺少这三个宏，编译链接都不会报错，但向量表仍指向启动文件的
 * 空函数 —— 任务切换与 tick 全部失效，表现为"上电后一个任务都不跑"。
 */
#define vPortSVCHandler				SVC_Handler
#define xPortPendSVHandler			PendSV_Handler
#define xPortSysTickHandler			SysTick_Handler

/*-----------------------------------------------------------
 * 断言与钩子函数声明
 *----------------------------------------------------------*/

/* 断言失败时关中断并死循环，便于在调试器中定位。
 * FreeRTOS 的配置类错误（优先级、堆溢出等）都靠它暴露，
 * 开发阶段务必保留。 */
#define configASSERT( x )	if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* 注意：hook 函数的原型声明不放在本文件。
 * 原因：本文件由 FreeRTOS.h 最先 include，此时 TaskHandle_t 等类型尚未定义，
 *       在此声明会导致 "identifier TaskHandle_t is undefined"。
 *       hook 实现与原型统一放在 BSP/freertos_hooks.c。 */

#endif /* FREERTOS_CONFIG_H */

