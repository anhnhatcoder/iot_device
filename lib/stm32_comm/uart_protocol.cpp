#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

#define RX 18
#define TX 17
#define BAUDRATE 115200

extern QueueHandle_t appManagerQueue;
extern QueueHandle_t stm32Queue;

uint8_t calculateChecksum(uint8_t cmd, uint8_t data) {
    return (0xAA ^ 0xBB ^ cmd ^ data);
}

void STM32CommTask(void *pvParameters) {
    Serial1.begin(BAUDRATE, SERIAL_8N1, RX, TX);

    AppMessage_t incomingMsg;
    AppMessage_t outgoingMsg;
    uint8_t rxBuffer[5];
    uint8_t rxIndex = 0;

    uint8_t buffer[5];

    while (1) {
        if(xQueueReceive(stm32Queue, &outgoingMsg, portMAX_DELAY) == pdTRUE) {
            uint8_t cmdByte = 0x00;
            uint8_t dataByte = (uint8_t)outgoingMsg.target_id;

            if((outgoingMsg.command == CMD_TURN_ON_RELAY) == pdTRUE) {
                cmdByte = 0x01;
            } else if((outgoingMsg.command == CMD_TURN_OFF_RELAY) == pdTRUE) {
                cmdByte = 0x02;
            } else if((outgoingMsg.command == CMD_TURN_ON_MOSFET) == pdTRUE) {
                cmdByte = 0x03;
            } else if((outgoingMsg.command == CMD_TURN_OFF_MOSFET) == pdTRUE) {
                cmdByte = 0x04; 
            }
        
         uint8_t txFrame[5] = {
            0xAA, 
            0xBB, 
            cmdByte, 
            dataByte, 
            calculateChecksum(cmdByte, dataByte) 
        };

        Serial1.write(txFrame, 5);

         }
        
         while (Serial1.available() > 0){
            uint8_t inByte = Serial1.read();
            
            if(rxIndex == 0 && inByte == 0xAA) continue;
            if(rxIndex == 1 && inByte == 0xBB) {rxIndex = 0; continue;}

            rxBuffer[rxIndex++] = inByte;

            if(rxIndex == 5) {
                uint8_t ReceivedChecksum = rxBuffer[4];
                uint8_t CalculatedChecksum = calculateChecksum(rxBuffer[2], rxBuffer[3]);
                
                if(ReceivedChecksum == CalculatedChecksum) {
                    uint8_t receivedCMD = rxBuffer[2];
                    uint8_t receivedData = rxBuffer[3];

                    incomingMsg.source = SRC_STM32;

                    if (receivedCMD == 0x03) {
                        incomingMsg.command = CMD_UPDATE_TEMP_HUMID;
                        incomingMsg.payload = receivedData * 10; // Đưa vào payload
                        
                        // Đẩy vào Queue cho App Manager xử lý
                        xQueueSend(appManagerQueue, &incomingMsg, 0);
                    }
                }
                
                // Reset bộ đệm để đón Frame tiếp theo
                rxIndex = 0; 
            }
        }

        // Nhường CPU cho các Task khác trên Core 0 (WiFi, MQTT...)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

                    
  
