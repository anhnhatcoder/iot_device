#include <Arduino.h>
#include <WiFiClientSecure.h> 
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include "shared_types.h"
#include "secrets.h"

const char* mqtt_server = "xxxxxxxxxxxxxx-ats.iot.ap-southeast-1.amazonaws.com"; 
const int   mqtt_port   = 8883;             
const char* client_id   = "ESP32_SmartGarden_Gateway";

WiFiClientSecure espClient; 
PubSubClient mqttClient(espClient);

extern QueueHandle_t appManagerQueue;
extern QueueHandle_t networkQueue;
extern EventGroupHandle_t networkEventGroup;

// [Hàm mqttCallback giữ nguyên logic của bạn]
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // ... phân tích lệnh và quăng vào appManagerQueue ...
}

void MQTTTask(void *pvParameters) {
    // --- BƯỚC QUAN TRỌNG: NẠP CHỨNG CHỈ BẢO MẬT ---
    espClient.setCACert(AWS_CERT_CA);
    espClient.setCertificate(AWS_CERT_CRT);
    espClient.setPrivateKey(AWS_CERT_PRIVATE);
    
    // Cấu hình MQTT
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    AppMessage_t incomingMsg;
    unsigned long lastMqttReconnectAttempt = 0;

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            networkEventGroup, 
            WIFI_CONNECTED_BIT, 
            pdFALSE, pdTRUE, pdMS_TO_TICKS(100)
        );

        if ((bits & WIFI_CONNECTED_BIT) != 0) {
            
            if (!mqttClient.connected()) {
                if (millis() - lastMqttReconnectAttempt > 5000) {
                    lastMqttReconnectAttempt = millis();
                    
                    // Kết nối không cần User/Pass vì đã dùng Private Key
                    if (mqttClient.connect(client_id)) { 
                        mqttClient.subscribe("gateway/control/#"); // Theo dõi tất cả lệnh điều khiển
                    }
                }
            } 
            else {
                mqttClient.loop(); 

                // Xử lý gửi dữ liệu lên AWS
                // Xử lý gửi dữ liệu lên AWS
                if (xQueueReceive(networkQueue, &incomingMsg, 0) == pdTRUE) {
                    char jsonBuffer[128];

                    if (incomingMsg.command == CMD_UPDATE_TEMP) {
                        float temp = (float)incomingMsg.payload / 10.0;
                        snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"temperature\": %.1f, \"device\": \"%s\"}", temp, client_id);
                        mqttClient.publish("gateway/sensor/data", jsonBuffer);
                    }
                    else if (incomingMsg.command == CMD_UPDATE_HUMID) {
                        float humid = (float)incomingMsg.payload / 10.0;
                        snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"humidity\": %.1f, \"device\": \"%s\"}", humid, client_id);
                        mqttClient.publish("gateway/sensor/data", jsonBuffer);
                    }
                    else if (incomingMsg.command == CMD_SYNC_RELAY_STATUS) {
                        snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"relay_%d\": %d}", incomingMsg.target_id, incomingMsg.payload);
                        mqttClient.publish("gateway/status/relay", jsonBuffer);
                    }
                    else if (incomingMsg.command == CMD_SYNC_MOSFET_STATUS) {
                        snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"mosfet_%d\": %d}", incomingMsg.target_id, incomingMsg.payload);
                        mqttClient.publish("gateway/status/mosfet", jsonBuffer);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}