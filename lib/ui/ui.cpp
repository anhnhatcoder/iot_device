#include "ui.h"
#include <Arduino.h>
#include <Wire.h>
#include "XPowersLib.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "shared_types.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "logo.c" 

// Extern các Queue từ main.cpp hoặc app_manager.cpp
extern QueueHandle_t appManagerQueue;
extern QueueHandle_t uiQueue;

// ==========================================
// 1. CẤU HÌNH LOVYANGFX 
// ==========================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796   _panel_instance; 
  lgfx::Bus_SPI        _bus_instance;
  lgfx::Light_PWM      _light_instance;
  lgfx::Touch_FT5x06   _touch_instance;
public:
  LGFX(void) {
    { auto cfg = _bus_instance.config(); cfg.spi_host = SPI3_HOST; cfg.spi_mode = 0; cfg.freq_write = 80000000; cfg.freq_read = 16000000; cfg.spi_3wire = false; cfg.use_lock = true; cfg.dma_channel = SPI_DMA_CH_AUTO; cfg.pin_sclk = 5; cfg.pin_mosi = 1; cfg.pin_miso = 2; cfg.pin_dc = 3; _bus_instance.config(cfg); _panel_instance.setBus(&_bus_instance); }
    { auto cfg = _panel_instance.config(); cfg.pin_cs = -1; cfg.pin_rst = -1; cfg.pin_busy = -1; cfg.memory_width = 320; cfg.panel_width = 320; cfg.memory_height = 480; cfg.panel_height = 480; cfg.offset_x = 0; cfg.offset_y = 0; cfg.offset_rotation = 0; cfg.dummy_read_pixel = 8; cfg.dummy_read_bits = 1; cfg.readable = true; cfg.invert = true; cfg.rgb_order = false; cfg.dlen_16bit = false; cfg.bus_shared = true; _panel_instance.config(cfg); }
    { auto cfg = _light_instance.config(); cfg.pin_bl = 6; cfg.invert = false; cfg.freq = 44100; cfg.pwm_channel = 7; _light_instance.config(cfg); _panel_instance.setLight(&_light_instance); }
    { auto cfg = _touch_instance.config(); cfg.x_min = 0; cfg.x_max = 319; cfg.y_min = 0; cfg.y_max = 479; cfg.pin_int = -1; cfg.bus_shared = false; cfg.offset_rotation = 0; cfg.i2c_port = 0; cfg.i2c_addr = 0x38; cfg.pin_sda = 8; cfg.pin_scl = 7; cfg.freq = 400000; _touch_instance.config(cfg); _panel_instance.setTouch(&_touch_instance); }
    setPanel(&_panel_instance);
  }
};

LGFX tft;
LGFX_Sprite ui(&tft); 
XPowersAXP2101 PMU;

// ==========================================
// 2. BIẾN QUẢN LÝ DỮ LIỆU
// ==========================================
enum Page { PAGE_HOME = 0, PAGE_MOSFET = 1, PAGE_RELAY = 2, PAGE_SENSOR = 3 };
Page currentPage = PAGE_HOME;

bool mosfetState[4]   = {false, true, false, false}; 
float mosfetCurrent[4] = {0.0, 1245.5, 0.0, 0.0}; 

bool relayState[4]  = {false, false, false, false};
float temp_C        = 30.2;   
float hum_RH        = 72.4;   

// ==========================================
// 3. KHỞI TẠO NGUỒN VÀ MÀN HÌNH (Gộp từ setup cũ)
// ==========================================
void init_power() {
  const uint8_t TCA9554_ADDR = 0x20; 
  Wire.beginTransmission(TCA9554_ADDR); Wire.write(0x03); Wire.write(0x75); Wire.endTransmission();
  Wire.beginTransmission(TCA9554_ADDR); Wire.write(0x01); Wire.write(0x88); Wire.endTransmission(); delay(50);
  Wire.beginTransmission(TCA9554_ADDR); Wire.write(0x01); Wire.write(0x8A); Wire.endTransmission(); delay(50);

  if (PMU.init(Wire, 8, 7, AXP2101_SLAVE_ADDRESS)) {
    PMU.setDC1Voltage(3300); PMU.setALDO1Voltage(3300); PMU.enableALDO1();
    PMU.setBLDO1Voltage(3300); PMU.enableBLDO1(); PMU.setBLDO2Voltage(3300); PMU.enableBLDO2();
  }
}

void UI_Init() {
  Wire.begin(8, 7);
  init_power();

  tft.init();
  tft.setRotation(1); 
  tft.setBrightness(255); 
  
  ui.setPsram(true); 
  ui.createSprite(480, 320); 
}

// ==========================================
// 4. VẼ NỘI DUNG TỪNG TRANG
// ==========================================
void drawPageContent(Page page, int offsetX) {
  ui.setTextDatum(middle_center);
  
  if (page != PAGE_HOME) {
    ui.fillRoundRect(offsetX + 15, 15 + 4, 80, 35, 8, tft.color565(30, 41, 59)); 
    ui.fillRoundRect(offsetX + 15, 15, 80, 35, 8, tft.color565(71, 85, 105));   
    ui.setTextColor(TFT_WHITE);
    ui.drawString("< BACK", offsetX + 55, 32, &fonts::FreeSansBold9pt7b);
  }

  if (page == PAGE_HOME) {
    ui.setTextColor(TFT_BLACK); ui.drawString("IOT MAIN DASHBOARD", offsetX + 242, 37, &fonts::FreeSansBold12pt7b);
    ui.setTextColor(TFT_WHITE); ui.drawString("IOT MAIN DASHBOARD", offsetX + 240, 35, &fonts::FreeSansBold12pt7b);

    ui.fillRoundRect(offsetX + 80, 75 + 5, 320, 60, 12, tft.color565(2, 90, 140)); 
    ui.fillRoundRect(offsetX + 80, 75, 320, 60, 12, tft.color565(14, 165, 233));   
    ui.drawString("1. MOSFET CONTROL", offsetX + 240, 105, &fonts::FreeSansBold12pt7b);

    ui.fillRoundRect(offsetX + 80, 155 + 5, 320, 60, 12, tft.color565(160, 90, 0)); 
    ui.fillRoundRect(offsetX + 80, 155, 320, 60, 12, tft.color565(245, 158, 11)); 
    ui.drawString("2. RELAY CONTROL", offsetX + 240, 185, &fonts::FreeSansBold12pt7b);

    ui.fillRoundRect(offsetX + 80, 235 + 5, 320, 60, 12, tft.color565(5, 110, 70)); 
    ui.fillRoundRect(offsetX + 80, 235, 320, 60, 12, tft.color565(16, 185, 129)); 
    ui.drawString("3. SENSOR DATA", offsetX + 240, 265, &fonts::FreeSansBold12pt7b);
  } 
  
  else if (page == PAGE_MOSFET) {
    ui.setTextColor(TFT_BLACK); ui.drawString("MOSFET CONTROL", offsetX + 262, 37, &fonts::FreeSansBold12pt7b);
    ui.setTextColor(TFT_WHITE); ui.drawString("MOSFET CONTROL", offsetX + 260, 35, &fonts::FreeSansBold12pt7b);

    for(int i = 0; i < 4; i++) {
      int bx = offsetX + 40 + (i * 105);
      int by = 100; 
      int bw = 90;
      int bh = 140; 
      bool isOn = mosfetState[i];
      
      int dx = isOn ? 2 : 0; 
      int dy = isOn ? 6 : 0; 

      if (!isOn) {
        ui.fillRoundRect(bx, by + 6, bw, bh, 12, tft.color565(30, 40, 50)); 
      }

      uint32_t c = isOn ? tft.color565(220, 38, 38) : tft.color565(71, 85, 105); 
      ui.fillRoundRect(bx + dx, by + dy, bw, bh, 12, c);
      
      ui.setTextColor(TFT_WHITE);
      ui.drawString("MOS " + String(i+1), bx + dx + 45, by + dy + 30, &fonts::FreeSansBold12pt7b);
      ui.drawString(isOn ? "ACTIVE" : "INACTIVE", bx + dx + 45, by + dy + 70, &fonts::FreeSansBold9pt7b);
      
      ui.setTextColor(TFT_YELLOW);
      ui.drawString(String(mosfetCurrent[i], 1) + " mA", bx + dx + 45, by + dy + 110, &fonts::FreeSansBold9pt7b);
    }
  }

  else if (page == PAGE_RELAY) {
    ui.setTextColor(TFT_BLACK); ui.drawString("RELAY CONTROL", offsetX + 262, 37, &fonts::FreeSansBold12pt7b);
    ui.setTextColor(TFT_WHITE); ui.drawString("RELAY CONTROL", offsetX + 260, 35, &fonts::FreeSansBold12pt7b);

    for(int i = 0; i < 4; i++) {
      int bx = offsetX + 40 + (i * 105);
      int by = 130;
      bool isOn = relayState[i];
      
      int dx = isOn ? 2 : 0;
      int dy = isOn ? 6 : 0;

      if (!isOn) {
        ui.fillRoundRect(bx, by + 6, 90, 120, 12, tft.color565(30, 40, 50)); 
      }

      uint32_t c = isOn ? tft.color565(16, 185, 129) : tft.color565(71, 85, 105); 
      ui.fillRoundRect(bx + dx, by + dy, 90, 120, 12, c);
      
      ui.setTextColor(TFT_WHITE);
      ui.drawString("RELAY " + String(i+1), bx + dx + 45, by + dy + 40, &fonts::FreeSansBold9pt7b); 
      ui.drawString(isOn ? "ON" : "OFF", bx + dx + 45, by + dy + 85, &fonts::FreeSansBold12pt7b);
    }
  }

  else if (page == PAGE_SENSOR) {
    ui.setTextColor(TFT_BLACK); ui.drawString("ENVIRONMENT SENSORS", offsetX + 262, 37, &fonts::FreeSansBold12pt7b);
    ui.setTextColor(TFT_WHITE); ui.drawString("ENVIRONMENT SENSORS", offsetX + 260, 35, &fonts::FreeSansBold12pt7b);

    ui.fillRoundRect(offsetX + 50, 110 + 6, 170, 140, 15, tft.color565(150, 20, 20)); 
    ui.fillRoundRect(offsetX + 50, 110, 170, 140, 15, tft.color565(239, 68, 68)); 
    ui.setTextColor(TFT_WHITE);
    ui.drawString("TEMPERATURE", offsetX + 135, 145, &fonts::FreeSansBold9pt7b); 
    ui.drawString(String(temp_C, 1) + " C", offsetX + 135, 205, &fonts::FreeSansBold18pt7b);

    ui.fillRoundRect(offsetX + 260, 110 + 6, 170, 140, 15, tft.color565(10, 100, 150)); 
    ui.fillRoundRect(offsetX + 260, 110, 170, 140, 15, tft.color565(14, 165, 233)); 
    ui.drawString("HUMIDITY", offsetX + 345, 145, &fonts::FreeSansBold9pt7b); 
    ui.drawString(String(hum_RH, 1) + " %", offsetX + 345, 205, &fonts::FreeSansBold18pt7b);
  }
}

// ==========================================
// 5. HIỆU ỨNG TRƯỢT
// ==========================================
void renderStatic() {
  ui.setSwapBytes(true); 
  ui.pushImage(0, 0, 480, 320, (const uint16_t*)logo_map); 
  ui.setSwapBytes(false);
  drawPageContent(currentPage, 0);
  ui.pushSprite(0, 0);
}

void slideToPage(Page newPage, bool slideLeft) {
  int step = slideLeft ? -80 : 80; 
  int startX = slideLeft ? 480 : -480;

  for (int offset = 0; abs(offset) < 480; offset -= step) {
    ui.setSwapBytes(true); 
    ui.pushImage(0, 0, 480, 320, (const uint16_t*)logo_map); 
    ui.setSwapBytes(false);
    
    drawPageContent(currentPage, offset);
    drawPageContent(newPage, startX + offset);

    ui.pushSprite(0, 0); 
  }
  
  currentPage = newPage;
  renderStatic(); 
}

// ==========================================
// 6. TASK FREERTOS ĐIỀU KHIỂN UI (Thay thế loop cũ)
// ==========================================
void UITask(void *pvParameters) {
  // Khởi tạo phần cứng màn hình
  UI_Init();
  renderStatic(); 

  bool wasTouched = false;
  int startX = 0, startY = 0;
  int lastX = 0, lastY = 0;

  AppMessage_t incomingMsg;
  AppMessage_t outgoingMsg;

  while (1) {
    // --- PHẦN A: LẮNG NGHE DỮ LIỆU TỪ HỆ THỐNG (Không block) ---
    if (uiQueue != NULL && xQueueReceive(uiQueue, &incomingMsg, 0) == pdTRUE) {
        bool needsRender = false;

        switch (incomingMsg.command) {
            case CMD_UPDATE_TEMP_HUMID:
                // Giả sử payload chứa nhiệt độ x10 (để tránh truyền float)
                temp_C = (float)incomingMsg.payload / 10.0;
                // Nếu muốn truyền cả độ ẩm, bạn có thể thiết kế payload dạng struct hoặc bitshift
                if (currentPage == PAGE_SENSOR) needsRender = true;
                break;
                
            case CMD_SYNC_MOSFET_CURRENT:
                if (incomingMsg.target_id >= 0 && incomingMsg.target_id < 4) {
                    mosfetCurrent[incomingMsg.target_id] = (float)incomingMsg.payload;
                    if (currentPage == PAGE_MOSFET) needsRender = true;
                }
                break;
                
            default:
                break;
        }

        if (needsRender) {
            renderStatic();
        }
    }

    // --- PHẦN B: QUÉT CẢM ỨNG ---
    uint16_t x, y;
    bool isTouched = tft.getTouch(&x, &y);

    if (isTouched) {
      if (!wasTouched) {
        startX = x;
        startY = y;
        wasTouched = true;
      }
      lastX = x;
      lastY = y;
    } 
    else {
      if (wasTouched) {
        wasTouched = false;
        
        int deltaX = lastX - startX;
        int deltaY = lastY - startY;

        if (deltaX < -60 && currentPage != PAGE_HOME) { 
          slideToPage(PAGE_HOME, true); 
        } 
        
        else if (abs(deltaX) < 20 && abs(deltaY) < 20) {
          int tapX = startX;
          int tapY = startY;

          if (currentPage != PAGE_HOME) {
            if (tapX > 15 && tapX < 95 && tapY > 15 && tapY < 50) {
              slideToPage(PAGE_HOME, true); 
              continue; // Nhảy qua vòng lặp tiếp theo
            }
          }

          if (currentPage == PAGE_HOME) {
            if (tapY > 75 && tapY < 135) slideToPage(PAGE_MOSFET, false); 
            else if (tapY > 155 && tapY < 215) slideToPage(PAGE_RELAY, false); 
            else if (tapY > 235 && tapY < 295) slideToPage(PAGE_SENSOR, false);
          }
          
          else if (currentPage == PAGE_MOSFET) {
            if (tapY > 100 && tapY < 240) {
              int mosfetIdx = -1;
              if (tapX > 40 && tapX < 130)  mosfetIdx = 0;
              if (tapX > 145 && tapX < 235) mosfetIdx = 1;
              if (tapX > 250 && tapX < 340) mosfetIdx = 2;
              if (tapX > 355 && tapX < 445) mosfetIdx = 3;

              if (mosfetIdx != -1) {
                // 1. Đảo trạng thái hiển thị nội bộ
                mosfetState[mosfetIdx] = !mosfetState[mosfetIdx];
                renderStatic(); 

                // 2. Báo cho App Manager
                outgoingMsg.source = SRC_UI;
                outgoingMsg.command = mosfetState[mosfetIdx] ? CMD_TURN_ON_MOSFET : CMD_TURN_OFF_MOSFET;
                outgoingMsg.target_id = mosfetIdx;
                if (appManagerQueue != NULL) xQueueSend(appManagerQueue, &outgoingMsg, 0);
              }
            }
          }
          
          else if (currentPage == PAGE_RELAY) {
            if (tapY > 130 && tapY < 250) {
              int relayIdx = -1;
              if (tapX > 40 && tapX < 130)   relayIdx = 0;
              if (tapX > 145 && tapX < 235)  relayIdx = 1;
              if (tapX > 250 && tapX < 340)  relayIdx = 2;
              if (tapX > 355 && tapX < 445)  relayIdx = 3;

              if (relayIdx != -1) {
                // 1. Đảo trạng thái hiển thị nội bộ
                relayState[relayIdx] = !relayState[relayIdx];
                renderStatic(); 

                // 2. Báo cho App Manager
                outgoingMsg.source = SRC_UI;
                outgoingMsg.command = relayState[relayIdx] ? CMD_TURN_ON_RELAY : CMD_TURN_OFF_RELAY;
                outgoingMsg.target_id = relayIdx;
                if (appManagerQueue != NULL) xQueueSend(appManagerQueue, &outgoingMsg, 0);
              }
            }
          }
        }
      }
    }
    
    // --- PHẦN C: NHƯỜNG CPU ---
    // Delay 20ms cho mỗi vòng quét cảm ứng/vẽ để không chiếm dụng 100% Core
    vTaskDelay(pdMS_TO_TICKS(20)); 
  }
}