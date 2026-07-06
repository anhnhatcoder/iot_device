#pragma once

// Khởi tạo các thành phần giao diện
void UI_Init();

// Task FreeRTOS chạy vòng lặp UI
void UITask(void *pvParameters);