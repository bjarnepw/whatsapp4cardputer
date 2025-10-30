#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <M5Cardputer.h>

void drawHeader(const char* title, uint16_t bgColor = 0x0451);
void drawFooter(const char* text, uint16_t bgColor = 0x2104);
void showStatusScreen(String title, String message);
int drawWrappedText(String text, int x, int y, int maxWidth, bool alignRight = false, uint16_t color = WHITE);

void drawMainMenu();
void drawWifiScanPage();
void drawPasswordInputPage();
void drawServerConfigPage();
void drawChatsPage();
void drawChatView();
void drawInputBox();
void drawSettingsPage();

#endif
