#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"

#define UART_FRAME_MAX 64                  /* 一帧最大字节数, 也是队列元素大小 */

void Uart_Init(void);                      /* 建队列: 启动调度器前调 */
uint8_t Uart_Send(const char *frame);      /* 入队一帧: 1=成功 0=丢弃 */
void uartTask(void *pvParameters);

#endif
