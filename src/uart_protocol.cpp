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


    while (1) {
        if(xQueueReceive(stm32Queue, &outgoingMsg, 0) == pdTRUE) {
            uint8_t cmdByte = 0x00;
            uint8_t dataByte = (uint8_t)outgoingMsg.target_id;

            if(outgoingMsg.command == CMD_TURN_ON_RELAY)  {
                cmdByte = 0x01;
            } else if(outgoingMsg.command == CMD_TURN_OFF_RELAY)  {
                cmdByte = 0x02;
            } else if(outgoingMsg.command == CMD_TURN_ON_MOSFET)  {
                cmdByte = 0x03;
            } else if(outgoingMsg.command == CMD_TURN_OFF_MOSFET)  {
                cmdByte = 0x04; }
            
        
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
            
            if(rxIndex == 0 && inByte != 0xAA) continue;
            if(rxIndex == 1 && inByte != 0xBB) {rxIndex = 0; continue;}

            rxBuffer[rxIndex++] = inByte;

            if(rxIndex == 5) {
                uint8_t ReceivedChecksum = rxBuffer[4];
                uint8_t CalculatedChecksum = calculateChecksum(rxBuffer[2], rxBuffer[3]);
                
                if(ReceivedChecksum == CalculatedChecksum) {
                    uint8_t receivedCMD = rxBuffer[2];
                    uint8_t receivedData = rxBuffer[3];

                    incomingMsg.source = SRC_STM32;

                    if (receivedCMD == 0x05) {
                        incomingMsg.command = CMD_UPDATE_TEMP;
                        incomingMsg.payload = receivedData * 10; 

                        xQueueSend(appManagerQueue, &incomingMsg, 0);
                    }else if (receivedCMD == 0x06) {
                        incomingMsg.command = CMD_UPDATE_HUMID;      // Mới: Độ ẩm
                        incomingMsg.payload = receivedData * 10; 
                        xQueueSend(appManagerQueue, &incomingMsg, 0);
                    }else if (receivedCMD == 0x07) {
                        incomingMsg.command = CMD_SYNC_RELAY_STATUS;
                        incomingMsg.target_id = receivedData >> 4; // Dịch phải 4 bit để lấy ID
                        incomingMsg.payload = receivedData & 0x0F; // Che đi 4 bit đầu để lấy Trạng thái
                        xQueueSend(appManagerQueue, &incomingMsg, 0);
                    }
                    // XÁC NHẬN TỪ MOSFET (Lệnh 0x08)
                    else if (receivedCMD == 0x08) {
                        incomingMsg.command = CMD_SYNC_MOSFET_STATUS;
                        incomingMsg.target_id = receivedData >> 4; 
                        incomingMsg.payload = receivedData & 0x0F; 
                        xQueueSend(appManagerQueue, &incomingMsg, 0);
                    }
                }
                
                
                rxIndex = 0; 
            }
        }

        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

                    
  
