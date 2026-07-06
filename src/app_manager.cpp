#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

extern QueueHandle_t appManagerQueue;
extern QueueHandle_t stm32Queue;   // Phải khai báo để chuyển lệnh xuống UART
extern QueueHandle_t networkQueue; // Phải khai báo để chuyển dữ liệu lên AWS
extern QueueHandle_t uiQueue;      // Phải khai báo để cập nhật màn hình

void AppManagerTask(void *pvParameters) {
    AppMessage_t incomingMsg;
    
    while (1) {
        // App Manager cứ đứng chờ thư mãi mãi (portMAX_DELAY), có thư mới làm việc
        if (xQueueReceive(appManagerQueue, &incomingMsg, portMAX_DELAY) == pdTRUE) {
            
            switch (incomingMsg.command) {
                
                // 1. NHÓM LỆNH ĐIỀU KHIỂN (Từ UI hoặc Web gửi xuống)
                case CMD_TURN_ON_RELAY:
                case CMD_TURN_OFF_RELAY:
                case CMD_TURN_ON_MOSFET:
                case CMD_TURN_OFF_MOSFET:
                    if (incomingMsg.source == SRC_UI || incomingMsg.source == SRC_NETWORK) {
                        // Chuyển lệnh thẳng xuống cho STM32 thực thi phần cứng
                        xQueueSend(stm32Queue, &incomingMsg, 0); 
                        
                        // (Tùy chọn) Gửi bản sao sang Network để báo trạng thái lên AWS
                        // xQueueSend(networkQueue, &incomingMsg, 0); 
                    }
                    break;

                // 2. LỆNH DỮ LIỆU CẢM BIẾN (Từ STM32 gửi lên)
                case CMD_UPDATE_TEMP:
                case CMD_UPDATE_HUMID:      
                    if (incomingMsg.source == SRC_STM32) {
                        xQueueSend(uiQueue, &incomingMsg, 0);
                        xQueueSend(networkQueue, &incomingMsg, 0); // MỞ DÒNG NÀY ĐỂ BÁO LÊN AWS
                    }
                    break;
                case CMD_SYNC_RELAY_STATUS: // <--- THÊM DÒNG NÀY 
                    if (incomingMsg.source == SRC_STM32) {
                        xQueueSend(uiQueue, &incomingMsg, 0);
                    }
                    break; 
                case CMD_SYNC_MOSFET_STATUS: 
                    if (incomingMsg.source == SRC_STM32) {
                        xQueueSend(uiQueue, &incomingMsg, 0);
                        xQueueSend(networkQueue, &incomingMsg, 0); // MỞ DÒNG NÀY ĐỂ ĐỒNG BỘ NÚT BẤM TRÊN WEB
                    }
                    break;
                
                // 3. LỆNH TRẠNG THÁI MẠNG (Từ WiFi Task gửi sang)
                case CMD_WIFI_STATUS: 
                    if (incomingMsg.source == SRC_NETWORK) {
                        // Báo cho Màn hình biết để đổi màu icon WiFi
                        xQueueSend(uiQueue, &incomingMsg, 0);
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}