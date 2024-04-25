/*
 * LED blink with FreeRTOS
 */
#include "controller.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include "stdio.h"
#include "pico/stdlib.h"
// To connect to the bluetooth module, use the following command:
// sudo rfcomm connect /dev/rfcomm1 00:21:13:01:01:E9


int main() {
    stdio_init_all();
    adc_setup();

    xQueueAdcm = xQueueCreate(2, sizeof(adc_t));
    xQueueBTN = xQueueCreate(2, sizeof(btns_f));
    xSemaphorePower = xSemaphoreCreateBinary();

    xTaskCreate(led_startup_task, "LED Startup Task", 256, NULL, 2, NULL);
    xTaskCreate(xm_task, "Xm Task", 256, NULL, 1, NULL);
    xTaskCreate(ym_task, "Ym Task", 256, NULL, 1, NULL);
    xTaskCreate(uartm_task, "UARTm Task", 256, NULL, 1, NULL);

    xTaskCreate(hc05_task, "HC05 Task", 256, NULL, 1, NULL);

    xTaskCreate(btn_task, "btn Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}