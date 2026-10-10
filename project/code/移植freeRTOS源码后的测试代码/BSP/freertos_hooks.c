/**
  ******************************************************************************
  * @file    freertos_hooks.c
  * @brief   FreeRTOS 回调钩子函数实现
  ******************************************************************************
  * @note    本文件集中实现 FreeRTOSConfig.h 中开启的三个 hook。
  *          单独成文件，避免与 app.c 的业务逻辑混杂。
  *
  *          各 hook 的使能开关（均在 FreeRTOSConfig.h）：
  *            vApplicationTickHook          <- configUSE_TICK_HOOK
  *            vApplicationStackOverflowHook <- configCHECK_FOR_STACK_OVERFLOW
  *            vApplicationMallocFailedHook  <- configUSE_MALLOC_FAILED_HOOK
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"

/**
  * @brief  FreeRTOS 每个 tick 中断的回调
  * @note   调用 HAL_IncTick() 驱动 HAL 时基。
  *
  *         背景：启用 FreeRTOS 后 SysTick_Handler 由 port.c 接管
  *         （见 stm32f1xx_it.c 中的说明），原先它调用的 HAL_IncTick()
  *         被一并移除。若不在此处补回，HAL_GetTick() 将恒为 0，
  *         导致所有基于 HAL_GetTick() 的时间判断失效。
  *
  * @warning 本函数在 SysTick 中断上下文中执行，务必保持简短，
  *          不得调用任何会阻塞或带 FromISR 后缀之外的 FreeRTOS API。
  */
void vApplicationTickHook(void)
{
	HAL_IncTick();        /* 驱动 HAL 时基(tick 计数) */
}

/**
  * @brief  任务栈溢出钩子
  * @param  xTask       溢出的任务句柄
  * @param  pcTaskName  任务名
  * @note   仅在 configCHECK_FOR_STACK_OVERFLOW 非 0 时被调用。
  *         栈溢出后任务已无法正常返回，此处死循环等待调试器介入。
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	(void)xTask;
	(void)pcTaskName;

	taskDISABLE_INTERRUPTS();
	for (;;)              /* 停在此处，便于调试器查看是哪条任务溢出 */
	{
	}
}

/**
  * @brief  堆分配失败钩子
  * @note   仅在 configUSE_MALLOC_FAILED_HOOK 非 0 时被调用。
  *         常见原因：configTOTAL_HEAP_SIZE 不足（创建任务/队列时耗尽）。
  *         可在此处加大 configTOTAL_HEAP_SIZE 或用 xPortGetFreeHeapSize()
  *         排查剩余堆容量。
  */
void vApplicationMallocFailedHook(void)
{
	taskDISABLE_INTERRUPTS();
	for (;;)              /* 停在此处，便于调试器定位 */
	{
	}
}
