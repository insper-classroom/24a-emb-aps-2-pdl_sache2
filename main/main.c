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
#include "hardware/adc.h"
#include <math.h>

#include "hc06.h"

#define Xm_AXIS_CHANNEL 26
#define Ym_AXIS_CHANNEL 27
#define Xf_AXIS_CHANNEL 28
#define Yf_AXIS_CHANNEL 29
#define BTN_A 2
#define BTN_B 3
#define BTN_X 4
#define BTN_Y 5
#define BTN_TL 6
#define BTN_TR 7
// #define BTN_SELECT
// #define BTN_START
#define BTN_THUMBL 8
#define BTN_THUMBR 9
#define BTN_DPAD_UP 10
#define BTN_DPAD_DOWN 11
#define BTN_DPAD_LEFT 12
#define BTN_DPAD_RIGHT 13
#define BTN_POWER 14

QueueHandle_t xQueueAdcm;
QueueHandle_t xQueueAdcf;
QueueHandle_t xQueueBTN;
SemaphoreHandle_t xSemaphorePower;

typedef struct adc {
    char axis;
    int val;
} adc_t;

void btnpower_callback(uint gpio, uint32_t events) {

    if (events == 0x4) {
        if (gpio == BTN_POWER){
            xSemaphoreGive(xSemaphorePower);
        }

    } 
}

void btn_callback(uint gpio, uint32_t events) {
    uint btnPressed = 0;

    if (events == 0x4) {  // Check if the interrupt event is a button press event
        if (gpio == BTN_A) {
            btnPressed = 0;  // Corresponds to "A" button
        } else if (gpio == BTN_B) {
            btnPressed = 1;  // Corresponds to "B" button
        } else if (gpio == BTN_X) {
            btnPressed = 2;  // Corresponds to "X" button
        } else if (gpio == BTN_Y) {
            btnPressed = 3;  // Corresponds to "Y" button
        } else if (gpio == BTN_TL) {
            btnPressed = 4;  // Corresponds to "TL" button
        } else if (gpio == BTN_TR) {
            btnPressed = 5;  // Corresponds to "TR" button
        } else if (gpio == BTN_THUMBL) {
            btnPressed = 6;  // Corresponds to "THUMBL" button (Left Thumb)
        } else if (gpio == BTN_THUMBR) {
            btnPressed = 7;  // Corresponds to "THUMBR" button (Right Thumb)
        } else if (gpio == BTN_DPAD_UP) {
            btnPressed = 8;  // Corresponds to "DPAD UP" button
        } else if (gpio == BTN_DPAD_DOWN) {
            btnPressed = 9; // Corresponds to "DPAD DOWN" button
        } else if (gpio == BTN_DPAD_LEFT) {
            btnPressed = 10; // Corresponds to "DPAD LEFT" button
        } else if (gpio == BTN_DPAD_RIGHT) {
            btnPressed = 11; // Corresponds to "DPAD RIGHT" button
        }
    }

    // Enqueue the button pressed
    xQueueSendFromISR(xQueueBTN, &btnPressed, NULL);
}

void init_buttons(void) {
    init_button(BTN_A);
    init_button(BTN_B);
    init_button(BTN_X);
    init_button(BTN_Y);
    init_button(BTN_TL);
    init_button(BTN_TR);
    init_button(BTN_THUMBL);
    init_button(BTN_THUMBR);
    init_button(BTN_DPAD_UP);
    init_button(BTN_DPAD_DOWN);
    init_button(BTN_DPAD_LEFT);
    init_button(BTN_DPAD_RIGHT);
}

// Function to initialize a single button
void init_button(uint pin) {
    gpio_init(pin);  // Initialize GPIO pin
    gpio_set_dir(pin, GPIO_IN);  // Set GPIO direction to input
    gpio_pull_up(pin);  // Enable pull-up resistor

    // Enable interrupt for the button
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart0, data.axis); //HC06_UART_ID
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, lsb);
}

void adc_setup() {
    adc_init();
    adc_gpio_init(Xm_AXIS_CHANNEL);
    adc_gpio_init(Ym_AXIS_CHANNEL);
    adc_gpio_init(Xf_AXIS_CHANNEL);
    adc_gpio_init(Yf_AXIS_CHANNEL);
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

int read_and_scale_adc(int axis) {
    int sum = 0;
    adc_select_input(axis);
    for (int i = 0; i < 5; i++) {
        sum += adc_read();
    }
    int avg = sum / 5;

    int scaled_val = ((avg - 2048) / 8);

    if ((scaled_val > -180) && (scaled_val < 180)) {
        scaled_val = 0; // Apply deadzone
    }

    return scaled_val / 16;
}

void xm_task(void *p) {
    adc_t data;
    data.axis = 12; // X-axis

    while (1) {
        data.val = read_and_scale_adc(0);
        xQueueSend(xQueueAdcm, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ym_task(void *p) {
    adc_t data;
    data.axis = 13; // Y-axis

    while (1) {
        data.val = read_and_scale_adc(1);
        xQueueSend(xQueueAdcm, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}

void xf_task(void *p) {
    adc_t data;
    data.axis = 14; // X-axis

    while (1) {
        data.val = read_and_scale_adc(2);
        xQueueSend(xQueueAdcf, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void yf_task(void *p) {
    adc_t data;
    data.axis = 15; // Y-axis

    while (1) {
        data.val = read_and_scale_adc(3);
        xQueueSend(xQueueAdcf, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
}

void uartm_task(void *p) {
    adc_t data;
    
    while (1) {
        if (xQueueReceive(xQueueAdcm, &data, portMAX_DELAY)) {
            write_package(data);
        }
    }
}

void uartf_task(void *p) {
    adc_t data;
    
    while (1) {
        if (xQueueReceive(xQueueAdcf, &data, portMAX_DELAY)) {
            write_package(data);
        }
    }
}

void btn_task(void *p) {
    init_buttons();
    uint BtnValue;
    adc_t data;
    
    while (1) {
        if (xQueueReceive(xQueueBTN, &BtnValue, portMAX_DELAY)) {
            data.axis = BtnValue;
            data.val = 1;
            write_package(data);
        }
    }
}

void led_task(void *p){
    gpio_init(BTN_POWER);
    gpio_set_dir(BTN_POWER, GPIO_OUT); 
    while(1){
        if (xSemaphoreTake(xSemaphorePower, portMAX_DELAY) == pdTRUE){
            gpio_put(BTN_POWER,1);
        }
    }
}

int main() {
    stdio_init_all();
    adc_setup();

    xQueueAdcm = xQueueCreate(32, sizeof(adc_t));
    xQueueAdcf = xQueueCreate(32, sizeof(adc_t));
    xQueueBTN = xQueueCreate(1, sizeof(u_int16_t));
    xSemaphorePower = xSemaphoreCreateBinary();

    xTaskCreate(xm_task, "Xm Task", 256, NULL, 1, NULL);
    xTaskCreate(ym_task, "Ym Task", 256, NULL, 1, NULL);
    xTaskCreate(uartm_task, "UARTm Task", 256, NULL, 1, NULL);

    xTaskCreate(xf_task, "Xf Task", 256, NULL, 1, NULL);
    xTaskCreate(yf_task, "Yf Task", 256, NULL, 1, NULL);
    xTaskCreate(uartf_task, "UARTf Task", 256, NULL, 1, NULL);

    xTaskCreate(btn_task, "btn Task", 256, NULL, 1, NULL);
    xTaskCreate(led_task, "led Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
