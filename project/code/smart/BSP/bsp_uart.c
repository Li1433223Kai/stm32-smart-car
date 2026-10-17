#include "bsp_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

static QueueHandle_t s_uartQueue = NULL;

void Uart_Init(void)
{
    s_uartQueue = xQueueCreate(4, UART_FRAME_MAX);   /* 深度 4, 每格 64 字节 */
}

uint8_t Uart_Send(const char *frame)
{
    if (s_uartQueue == NULL)
    {
        return 0;
    }
    return (xQueueSend(s_uartQueue, frame, 0) == pdTRUE) ? 1 : 0;   /* 超时 0: 满就丢 */
}

void uartTask(void *pvParameters)
{
    char buf[UART_FRAME_MAX];

    (void) pvParameters;

    for (; ;)
    {
        xQueueReceive(s_uartQueue, buf, portMAX_DELAY);   /* 没数据就阻塞, 不占 CPU */
        printf("%s", buf);
    }
}

