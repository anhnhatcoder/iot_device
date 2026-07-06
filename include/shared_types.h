#pragma once
#include <stdint.h>

// định nghĩa giao tiếp với app manager
typedef enum {
    SRC_UI,
    SRC_NETWORK,
    SRC_STM32,
} EventSource_t;

typedef enum {
    CMD_TURN_ON_RELAY,
    CMD_TURN_OFF_RELAY,
    CMD_TURN_ON_MOSFET,
    CMD_TURN_OFF_MOSFET,
    CMD_UPDATE_TEMP_HUMID,
    CMD_WIFI_STATUS, 
    CMD_SYNC_MOSFET_CURRENT,   
} Command_t;

typedef struct {
    EventSource_t source;
    Command_t command;
    uint8_t target_id; 
    uint32_t payload; 
} AppMessage_t;

extern QueueHandle_t appManagerQueue;
extern QueueHandle_t uiQueue;
extern QueueHandle_t stm32Queue;