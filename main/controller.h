#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <string.h>

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

typedef struct {
    char axis;
    int val;
} adc_t;

typedef struct {
    int btnPressed;
    int value;
} btns_f;

void init_buttons(void);
void adc_setup(void);
void gpio_init_pins(void);

void led_startup_task(void *p);
void xm_task(void *p);
void ym_task(void *p);
void uartm_task(void *p);
void hc05_task(void *p);
void btn_task(void *p);
void beep(int pin, int freq, int time);

extern QueueHandle_t xQueueAdcm;
extern QueueHandle_t xQueueBTN;
extern SemaphoreHandle_t xSemaphorePower;

#endif
