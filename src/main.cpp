#include <Arduino.h>
#include <Wire.h>
#include "XPowersLib.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "logo.c" 

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
// Mảng lưu dòng điện riêng cho 4 MOSFET (mA)
float mosfetCurrent[4] = {0.0, 1245.5, 0.0, 0.0}; 

bool relayState[4]  = {false, false, false, false};
float temp_C        = 30.2;   
float hum_RH        = 72.4;   

// ==========================================
// 3. KHỞI TẠO NGUỒN
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
      int by = 100; // Đẩy nút lên cao một chút
      int bw = 90;
      int bh = 140; // Nút dài ra để chứa thêm số mA
      bool isOn = mosfetState[i];
      
      int dx = isOn ? 2 : 0; 
      int dy = isOn ? 6 : 0; 

      if (!isOn) {
        ui.fillRoundRect(bx, by + 6, bw, bh, 12, tft.color565(30, 40, 50)); 
      }

      uint32_t c = isOn ? tft.color565(220, 38, 38) : tft.color565(71, 85, 105); 
      ui.fillRoundRect(bx + dx, by + dy, bw, bh, 12, c);
      
      // Tên Mosfet
      ui.setTextColor(TFT_WHITE);
      ui.drawString("MOS " + String(i+1), bx + dx + 45, by + dy + 30, &fonts::FreeSansBold12pt7b);
      // Trạng thái
      ui.drawString(isOn ? "ACTIVE" : "INACTIVE", bx + dx + 45, by + dy + 70, &fonts::FreeSansBold9pt7b);
      
      // Dòng điện độc lập (Màu vàng)
      ui.setTextColor(TFT_YELLOW);
      // Giới hạn 1 chữ số thập phân cho đẹp
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
    ui.drawString(String(temp_C, 1) + "  C", offsetX + 135, 205, &fonts::FreeSansBold18pt7b);

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
// 6. SETUP & VÒNG LẶP ĐIỀU KHIỂN
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(8, 7);
  init_power();

  tft.init();
  tft.setRotation(1); 
  tft.setBrightness(255); 
  
  ui.setPsram(true); 
  ui.createSprite(480, 320); 

  renderStatic(); 
}

void loop() {
  static bool wasTouched = false;
  static int startX = 0, startY = 0;
  static int lastX = 0, lastY = 0;

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
            return; 
          }
        }

        if (currentPage == PAGE_HOME) {
          if (tapY > 75 && tapY < 135) slideToPage(PAGE_MOSFET, false); 
          else if (tapY > 155 && tapY < 215) slideToPage(PAGE_RELAY, false); 
          else if (tapY > 235 && tapY < 295) slideToPage(PAGE_SENSOR, false);
        }
        
        else if (currentPage == PAGE_MOSFET) {
          // Tọa độ Y nút MOSFET đã đổi thành 100 -> 240
          if (tapY > 100 && tapY < 240) {
            if (tapX > 40 && tapX < 130)  { 
              mosfetState[0] = !mosfetState[0]; 
              mosfetCurrent[0] = mosfetState[0] ? 350.5 : 0.0; // Data ảo
            }
            if (tapX > 145 && tapX < 235) { 
              mosfetState[1] = !mosfetState[1]; 
              mosfetCurrent[1] = mosfetState[1] ? 1245.5 : 0.0; 
            }
            if (tapX > 250 && tapX < 340) { 
              mosfetState[2] = !mosfetState[2]; 
              mosfetCurrent[2] = mosfetState[2] ? 85.0 : 0.0; 
            }
            if (tapX > 355 && tapX < 445) { 
              mosfetState[3] = !mosfetState[3]; 
              mosfetCurrent[3] = mosfetState[3] ? 1024.1 : 0.0; 
            }
            renderStatic(); 
          }
        }
        
        else if (currentPage == PAGE_RELAY) {
          if (tapY > 130 && tapY < 250) {
            if (tapX > 40 && tapX < 130)   relayState[0] = !relayState[0];
            if (tapX > 145 && tapX < 235)  relayState[1] = !relayState[1];
            if (tapX > 250 && tapX < 340)  relayState[2] = !relayState[2];
            if (tapX > 355 && tapX < 445)  relayState[3] = !relayState[3];
            renderStatic(); 
          }
        }
      }
    }
  }
  delay(10); 
}