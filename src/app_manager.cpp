#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

extern QueueHandle_t appManagerQueue;

void AppManagerTask(void *pvParameters) {
  AppMessage_t incomingMsg;
  while (1) {
    if (xQueueReceive(appManagerQueue, &incomingMsg, 0) == pdTRUE) {
            
            // Xử lý dựa trên lệnh (Command) nhận được
            switch (incomingMsg.command) {
                
                case CMD_TURN_ON_RELAY:
                    if (incomingMsg.source == SRC_UI) {
                        // 1. UI báo bật -> Gửi lệnh xuống STM32
                        // STM32_Send_Command(TURN_ON, incomingMsg.payload);
                        
                        // 2. Publish MQTT lên Web Portal báo đang xử lý
                        // Network_Publish_MQTT("device/relay", "ON");
                    }
                    break;

                case CMD_UPDATE_TEMP_HUMID:
                    if (incomingMsg.source == SRC_STM32) {
                        // STM32 gửi nhiệt độ lên -> Cập nhật màn hình UI
                        // UI_Update_Label(incomingMsg.payload);
                        
                        // Gửi lên server
                        // Network_Publish_MQTT("device/sensor", incomingMsg.payload);
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}


