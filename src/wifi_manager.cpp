#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

const char* ssid = "Chocomeo";
const char* password = "11111111";

extern QueueHandle_t appManagerQueue;
extern EventGroupHandle_t networkEventGroup;

void WiFiTask(void *pvParameters) {
    // Tự động kết nối lại ở mức nhân phần cứng (Hardware level)
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    bool wasConnected = false;
    unsigned long lastWifiCheck = 0;

    Serial.println("WiFi: Đang khoi tao ket noi...");
    WiFi.begin(ssid, password);

    while (1) {
        if (millis() - lastWifiCheck >= 5000) {
            lastWifiCheck = millis();
            bool isConnected = (WiFi.status() == WL_CONNECTED);

            // 1. KIỂM TRA SỰ THAY ĐỔI: Nếu có sự cố / Hoặc vừa kết nối lại thành công
            if (isConnected != wasConnected) {
                wasConnected = isConnected;
                
                // Báo cho "Nhạc trưởng" (App Manager) để chớp đèn UI
                AppMessage_t statusMsg;
                statusMsg.source = SRC_NETWORK;
                statusMsg.command = CMD_WIFI_STATUS;
                statusMsg.payload = isConnected ? 1 : 0;
                if (appManagerQueue != NULL) xQueueSend(appManagerQueue, &statusMsg, 0);

                // Cập nhật Cột đèn giao thông (EventGroup)
                if (isConnected) {
                    Serial.println("WiFi: DA KET NOI!");
                    xEventGroupSetBits(networkEventGroup, WIFI_CONNECTED_BIT); 
                } else {
                    Serial.println("WiFi: MAT KET NOI!");
                    xEventGroupClearBits(networkEventGroup, WIFI_CONNECTED_BIT); 
                }
            }

            // 2. ÉP BUỘC KẾT NỐI: Nếu vẫn đang rớt mạng, cứ 5 giây "gõ cửa" router 1 lần
            if (!isConnected) {
                Serial.println("WiFi: Dang co gang ket noi lai...");
                WiFi.reconnect(); 
            }
        }
        
        // Nhường CPU cho các Task khác
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}