#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

void AppManagerTask(void *pvParameters);
void STM32CommTask(void *pvParameters);




QueueHandle_t appManagerQueue;
QueueHandle_t uiQueue;
QueueHandle_t stm32Queue;



void setup() {
    appManagerQueue = xQueueCreate(20, sizeof(AppMessage_t));
    uiQueue = xQueueCreate(20, sizeof(AppMessage_t));
    stm32Queue = xQueueCreate(20, sizeof(AppMessage_t));

    xTaskCreatePinnedToCore(AppManagerTask, "AppManagerTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(STM32CommTask, "STM32CommTask", 4096, NULL, 1, NULL, 1);   
}