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
#include "hardware/pwm.h"
#include <math.h>

#include "hc06.h"

#define Xm_AXIS_CHANNEL 26
#define Ym_AXIS_CHANNEL 27
#define KEY_W 3
#define KEY_S 16
#define KEY_D 17
#define KEY_A 6
#define KEY_SPACE 7
#define KEY_UP 8
#define KEY_DOWN 9
#define KEY_LEFT 10
#define KEY_RIGHT 11
#define BTN_LEFT 12
#define BTN_RIGHT 13
#define BUZZER 14
#define LED_GREEN 15
#define BLUETOOTH_TX_PIN 5
#define BLUETOOTH_RX_PIN 4

QueueHandle_t xQueueAdcm;
QueueHandle_t xQueueAdcf;
QueueHandle_t xQueueBTN;
SemaphoreHandle_t xSemaphorePower;

typedef struct adc {
    char axis;
    int val;
} adc_t;

typedef struct btns{
    char btnPressed;
    int value;
} btns_f;

void beep(int pin, int freq, int time){
    gpio_init(BUZZER);  // Initialize GPIO pin
    gpio_set_dir(BUZZER, GPIO_OUT); 
    long delay = 1000000 / (2 * freq);
    long cycles = (long)freq * time / 1000;
    for (long i = 0; i < cycles; i++){
        gpio_put(pin, 1);
        sleep_us(delay);
        gpio_put(pin, 0);
        sleep_us(delay);
    }
}

void led_startup_task(void *p) {
    gpio_init(LED_GREEN);          
    gpio_set_dir(LED_GREEN, GPIO_OUT);  

    gpio_put(LED_GREEN, 1);         
    beep(BUZZER, 1000, 1000);       
    vTaskDelay(pdMS_TO_TICKS(3000));
    gpio_put(LED_GREEN, 0);         

    vTaskDelete(NULL);             
}

// void btnpower_callback(uint gpio, uint32_t events) {

//     if (events == 0x4) {
//         if (gpio == BTN_POWER){
//             xSemaphoreGive(xSemaphorePower);

//         }

//     } 
// }

void btn_callback(uint gpio, uint32_t events) {
    btns_f buttons;

    if (events == 0x4) { 
        buttons.value=3; // Check if the interrupt event is a button press event
        if (gpio == KEY_W) {
            buttons.btnPressed = 1;  // Corresponds to "A" button
        } if (gpio == KEY_S) {
            buttons.btnPressed = 3;  // Corresponds to "X" button
        } if (gpio == KEY_D) {
            buttons.btnPressed = 4;  // Corresponds to "Y" button
        } if (gpio == KEY_A) {
            buttons.btnPressed = 2;  // Corresponds to "TL" button
        } if (gpio == KEY_SPACE) {
            buttons.btnPressed = 5;  // Corresponds to "TR" button
        } if (gpio == KEY_UP) {
            buttons.btnPressed = 6;  // Corresponds to "THUMBL" button (Left Thumb)
        } if (gpio == KEY_DOWN) {
            buttons.btnPressed = 7;  // Corresponds to "THUMBR" button (Right Thumb)
        } if (gpio == KEY_LEFT) {
            buttons.btnPressed = 8;  // Corresponds to "DPAD UP" button
        } if (gpio == KEY_RIGHT) {
            buttons.btnPressed = 9; // Corresponds to "DPAD DOWN" button
        } if (gpio == BTN_LEFT) {
            buttons.btnPressed = 10; // Corresponds to "DPAD LEFT" button
        } if (gpio == BTN_RIGHT) {
            buttons.btnPressed = 11; // Corresponds to "DPAD RIGHT" button
        }
        xQueueSendFromISR(xQueueBTN, &buttons, NULL);
    }
    if (events == 0x8){
        buttons.value=2; // Check if the interrupt event is a button press event
        if (gpio == KEY_W) {
            buttons.btnPressed = 1;  // Corresponds to "A" button
        } if (gpio == KEY_S) {
            buttons.btnPressed = 3;  // Corresponds to "X" button
        } if (gpio == KEY_D) {
            buttons.btnPressed = 4;  // Corresponds to "Y" button
        } if (gpio == KEY_A) {
            buttons.btnPressed = 2;  // Corresponds to "TL" button
        } if (gpio == KEY_SPACE) {
            buttons.btnPressed = 5;  // Corresponds to "TR" button
        } if (gpio == KEY_UP) {
            buttons.btnPressed = 6;  // Corresponds to "THUMBL" button (Left Thumb)
        } if (gpio == KEY_DOWN) {
            buttons.btnPressed = 7;  // Corresponds to "THUMBR" button (Right Thumb)
        } if (gpio == KEY_LEFT) {
            buttons.btnPressed = 8;  // Corresponds to "DPAD UP" button
        } if (gpio == KEY_RIGHT) {
            buttons.btnPressed = 9; // Corresponds to "DPAD DOWN" button
        } if (gpio == BTN_LEFT) {
            buttons.btnPressed = 10; // Corresponds to "DPAD LEFT" button
        } if (gpio == BTN_RIGHT) {
            buttons.btnPressed = 11; // Corresponds to "DPAD RIGHT" button
        }
        xQueueSendFromISR(xQueueBTN, &buttons, NULL);

    }

    // Enqueue the button pressed
    
}

// Function to initialize a single button
void init_button(uint pin) {
    gpio_init(pin);  // Initialize GPIO pin
    gpio_set_dir(pin, GPIO_IN);  // Set GPIO direction to input
    gpio_pull_up(pin);  // Enable pull-up resistor

    // Enable interrupt for the button
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}

void init_buttons(void) {
    init_button(KEY_A);
    init_button(KEY_D);
    init_button(KEY_DOWN);
    init_button(KEY_LEFT);
    init_button(KEY_RIGHT);
    init_button(KEY_S);
    init_button(KEY_SPACE);
    init_button(KEY_UP);
    init_button(KEY_W);
    init_button(BTN_LEFT);
    init_button(BTN_RIGHT);
}


void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart0, data.axis); //HC06_UART_ID
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, -1);

}

void adc_setup() {
    adc_init();
    adc_gpio_init(Xm_AXIS_CHANNEL);
    adc_gpio_init(Ym_AXIS_CHANNEL);
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

    return scaled_val / 64;
}

void xm_task(void *p) {
    adc_t data;
    data.axis = 0;
     // X-axis
    

    while (1) {
        
        data.val = - read_and_scale_adc(0);
        //printf("valor: %d\n",data.val);
        //printf("indice e valor x do joystick: %d, %d\n",data.axis,data.val);
        if (data.val != 0)
            xQueueSend(xQueueAdcm, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ym_task(void *p) {
    adc_t data;
    data.axis = 1; // Y-axis

    while (1) {
        data.val = read_and_scale_adc(1);
        //printf("indice e valor y do joystick: %d, %d\n",data.axis,data.val);
        if (data.val != 0)
            xQueueSend(xQueueAdcm, &data, portMAX_DELAY);
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

// void uartf_task(void *p) {
//     adc_t data;
    
//     while (1) {
//         if (xQueueReceive(xQueueAdcf, &data, portMAX_DELAY)) {
//             write_package(data);
//         }
//     }
// }

void btn_task(void *p) {
    init_buttons();  // Assegura que os botões estão inicializados
    
    btns_f buttons;
    adc_t data;
    static TickType_t last_press_time[14] = {0};  // Array para armazenar o último tempo de pressionamento de cada botão
    TickType_t current_time;
    const TickType_t debounce_delay = pdMS_TO_TICKS(300);  // Conversão de tempo de debounce para ticks

    while (1) {
        if (xQueueReceive(xQueueBTN, &buttons, portMAX_DELAY)) {
            current_time = xTaskGetTickCount();  // Pega o tempo atual em ticks

            // Verifica se o intervalo desde o último pressionamento é maior que o tempo de debounce
            if (current_time - last_press_time[buttons.btnPressed] >= debounce_delay) {
                last_press_time[buttons.btnPressed] = current_time;  // Atualiza o último tempo de pressionamento para o botão atual
                
                // Prepara o pacote de dados para ser enviado
                data.val = buttons.btnPressed;
                data.axis = buttons.value;
                write_package(data);  // Envio do pacote
            }
        }
    }
}


int main() {
    stdio_init_all();
    adc_setup();

    xQueueAdcm = xQueueCreate(32, sizeof(adc_t));
    xQueueAdcf = xQueueCreate(32, sizeof(adc_t));
    xQueueBTN = xQueueCreate(32, sizeof(btns_f));
    xSemaphorePower = xSemaphoreCreateBinary();

    xTaskCreate(led_startup_task, "LED Startup Task", 256, NULL, 2, NULL);
    xTaskCreate(xm_task, "Xm Task", 256, NULL, 1, NULL);
    xTaskCreate(ym_task, "Ym Task", 256, NULL, 1, NULL);
    xTaskCreate(uartm_task, "UARTm Task", 256, NULL, 1, NULL);

    // xTaskCreate(hc06_task, "HC06 Task", 256, NULL, 1, NULL);

    //xTaskCreate(uartf_task, "UARTf Task", 256, NULL, 1, NULL);

    xTaskCreate(btn_task, "btn Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}