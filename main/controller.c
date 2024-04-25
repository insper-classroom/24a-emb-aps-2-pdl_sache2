#include "controller.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "hc05.h"

QueueHandle_t xQueueAdcm;
QueueHandle_t xQueueBTN;
SemaphoreHandle_t xSemaphorePower;

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
    vTaskDelay(pdMS_TO_TICKS(5000));
    gpio_put(LED_GREEN, 0);         

    vTaskDelete(NULL);             
}


void btn_callback(uint gpio, uint32_t events) {
    btns_f buttons;

    if (events == 0x4){ 
        buttons.value=3;// Check if the interrupt event is a button press event
        if (gpio == KEY_W) {
            buttons.btnPressed = 1; // Corresponds to W key
        } else if (gpio == KEY_S) { 
            buttons.btnPressed = 3; // Corresponds to S key 
        } else if (gpio == KEY_D) {
            buttons.btnPressed = 4; // Corresponds to D key
        } else if (gpio == KEY_A) {
            buttons.btnPressed = 2;  // Corresponds to A key
        } else if (gpio == KEY_SPACE) {
            buttons.btnPressed = 5;  // Corresponds to SPACE key
        } else if (gpio == KEY_UP) {
            buttons.btnPressed = 6; // Corresponds to UP ARROW key
        } else if (gpio == KEY_DOWN) {
            buttons.btnPressed = 7;  // Corresponds to DOWN ARROW key
        } else if (gpio == KEY_LEFT) {
            buttons.btnPressed = 8;  // Corresponds to LEFT ARROW key
        } else if (gpio == KEY_RIGHT) {
            buttons.btnPressed = 9; // Corresponds to RIGHT ARROW key
        } else if (gpio == BTN_LEFT) {
            buttons.btnPressed = 10; // Corresponds to MOUSE LEFT CLICK
        } else if (gpio == BTN_RIGHT) {
            buttons.btnPressed = 11; // Corresponds to MOUSE RIGHT CLICK
        }
    } else if (events == 0x8){
        buttons.value=2;
        if (gpio == KEY_W) {
            buttons.btnPressed = 1;  // Corresponds to W key
        } else if (gpio == KEY_S) {
            buttons.btnPressed = 3;  // Corresponds to S key
        } else if (gpio == KEY_D) {
            buttons.btnPressed = 4;  // Corresponds to D key
        } else if (gpio == KEY_A) {
            buttons.btnPressed = 2;  // Corresponds to A key
        } else if (gpio == KEY_SPACE) {
            buttons.btnPressed = 5;  // Corresponds to SPACE key
        } else if (gpio == KEY_UP) {
            buttons.btnPressed = 6;  // Corresponds to UP ARROW key
        } else if (gpio == KEY_DOWN) {
            buttons.btnPressed = 7;  // Corresponds to DOWN ARROW key
        } else if (gpio == KEY_LEFT) {
            buttons.btnPressed = 8;  // Corresponds to LEFT ARROW key
        } else if (gpio == KEY_RIGHT) {
            buttons.btnPressed = 9; // Corresponds to RIGHT ARROW key
        } else if (gpio == BTN_LEFT) {
            buttons.btnPressed = 10; // Corresponds to MOUSE LEFT CLICK
        } else if (gpio == BTN_RIGHT) {
            buttons.btnPressed = 11; // Corresponds to MOUSE RIGHT CLICK
        }
    }
    xQueueSendFromISR(xQueueBTN, &buttons, NULL);
    
}

// Function to initialize a single button
void init_button(uint pin) {
    gpio_init(pin);  // Initialize GPIO pin
    gpio_set_dir(pin, GPIO_IN);  // Set GPIO direction to input
    gpio_pull_up(pin);  // Enable pull-up resistor

    // Enable interrupt for the button
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}

// Function to initialize a single button
void init_button2(uint pin) {
    gpio_init(pin);  // Initialize GPIO pin
    gpio_set_dir(pin, GPIO_IN);  // Set GPIO direction to input
    gpio_pull_up(pin);  // Enable pull-up resistor

    // Enable interrupt for the button
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

void init_buttons(void) {
    init_button(KEY_A);
    init_button2(KEY_D);
    init_button2(KEY_DOWN);
    init_button2(KEY_LEFT);
    init_button2(KEY_RIGHT);
    init_button2(KEY_S);
    init_button2(KEY_SPACE);
    init_button2(KEY_UP);
    init_button2(KEY_W);
    init_button2(BTN_LEFT);
    init_button2(BTN_RIGHT);
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    // uart1 to send with bluetooth
    // uart0 to send with usb
    uart_putc_raw(uart1, data.axis);
    uart_putc_raw(uart1, msb);
    uart_putc_raw(uart1, lsb);
    uart_putc_raw(uart1, -1);

}

void adc_setup() {
    adc_init();
    adc_gpio_init(Xm_AXIS_CHANNEL);
    adc_gpio_init(Ym_AXIS_CHANNEL);
}


void hc05_task(void *p) {
    uart_init(hc05_UART_ID, hc05_BAUD_RATE);
    gpio_set_function(hc05_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(hc05_RX_PIN, GPIO_FUNC_UART);
    //hc05_init("pdl-sache", "1234");

    while(true){
        vTaskDelay(pdMS_TO_TICKS(1));
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
    data.axis = 0; // X-axis
    
    while (1) {
        
        data.val = - read_and_scale_adc(0);
        if (data.val != 0)
            xQueueSend(xQueueAdcm, &data, 1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void ym_task(void *p) {
    adc_t data;
    data.axis = 1; // Y-axis

    while (1) {
        data.val = read_and_scale_adc(1);
        if (data.val != 0)
            xQueueSend(xQueueAdcm, &data, 1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
}

void uartm_task(void *p) {
    adc_t data;
    
    while (1) {
        if (xQueueReceive(xQueueAdcm, &data, 1)) {
            write_package(data);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
    }
}

void btn_task(void *p) {
    init_buttons();  
    
    btns_f buttons;
    adc_t data; 

    while (1) {
        if (xQueueReceiveFromISR(xQueueBTN, &buttons, pdMS_TO_TICKS(1))) {
            data.val = buttons.btnPressed;
            data.axis = buttons.value;
            write_package(data);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}