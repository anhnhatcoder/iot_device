#include <Arduino.h>
#include <wiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

const char* ssid = "Na Bull";
const char* password = "0902523723";

extern QueueHandle_t appManagerQueue;
extern EventGroupHandle_t networkEventGroup;

void WiFiTask(void *pvParameters) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    bool wasConnected = false;
    unsigned long lastWifiCheck = 0;

    WiFi.begin(ssid, password);

    while (1) {
        if (millis() - lastWifiCheck >= 5000) {
            lastWifiCheck = millis();
            bool isConnected = (WiFi.status() == WL_CONNECTED);

            if (isConnected != wasConnected) {
                wasConnected = isConnected;
                
                
                AppMessage_t statusMsg;
                statusMsg.source = SRC_NETWORK;
                statusMsg.command = CMD_WIFI_STATUS;
                statusMsg.payload = isConnected ? 1 : 0;
                if (appManagerQueue != NULL) xQueueSend(appManagerQueue, &statusMsg, 0);

                
                if (isConnected) {
                    xEventGroupSetBits(networkEventGroup, WIFI_CONNECTED_BIT); // Bật đèn Xanh
                } else {
                    xEventGroupClearBits(networkEventGroup, WIFI_CONNECTED_BIT); // Bật đèn Đỏ
                    WiFi.disconnect();
                    WiFi.begin(ssid, password);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}