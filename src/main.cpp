#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include "shared_types.h"
#include "ui.h"

void AppManagerTask(void *pvParameters);
void STM32CommTask(void *pvParameters);
void WiFiTask(void *pvParameters);
void MQTTTask(void *pvParameters);
void UITask(void *pvParameters);



QueueHandle_t appManagerQueue;
QueueHandle_t uiQueue;
QueueHandle_t stm32Queue;
QueueHandle_t networkQueue;
EventGroupHandle_t networkEventGroup;


void setup() {
Serial.begin(115200);

    appManagerQueue = xQueueCreate(20, sizeof(AppMessage_t));
    uiQueue = xQueueCreate(20, sizeof(AppMessage_t));
    stm32Queue = xQueueCreate(20, sizeof(AppMessage_t));
    networkQueue = xQueueCreate(20, sizeof(AppMessage_t));
    networkEventGroup = xEventGroupCreate();

    xTaskCreatePinnedToCore(AppManagerTask, "AppManagerTask", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(STM32CommTask, "STM32CommTask", 4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(WiFiTask, "WiFiTask", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(MQTTTask, "MQTTTask", 8192, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(UITask, "UITask", 32768, NULL, 1, NULL, 1);
}

void loop() {
    
    vTaskDelete(NULL);
} 