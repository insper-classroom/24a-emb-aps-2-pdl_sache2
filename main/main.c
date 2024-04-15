/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <string.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hc06.h"

typedef struct adc {
    char axis;
    int val;
} adc_t;

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(HC06_UART_ID, data.axis);
    uart_putc_raw(HC06_UART_ID, msb);
    uart_putc_raw(HC06_UART_ID, lsb);
}


void hc06_task(void *p) {
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("pdl-sache", "1234");

    while (true) {
        uart_puts(HC06_UART_ID, "OLAAA ");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    printf("Start bluetooth task\n");

    xTaskCreate(hc06_task, "UART_Task 1", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
