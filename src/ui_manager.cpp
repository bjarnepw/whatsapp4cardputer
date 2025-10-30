#include "ui_manager.h"
#include "config.h"
#include <WiFi.h>

//TODO: MAKE THE SHOW PASSWORD THING WORK


// GLOBAL VARIABLES
bool showPassword = true;
bool togglePasswordPressed = false; // to detect press


void drawHeader(const char* title, uint16_t bgColor) {
    auto& lcd = M5.Display;
    lcd.fillRect(0, 0, lcd.width(), 18, bgColor);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.drawString(title, lcd.width() / 2, 9);
}

void drawFooter(const char* text, uint16_t bgColor) {
    auto& lcd = M5.Display;
    int footerHeight = 14;
    int footerY = lcd.height() - footerHeight;
    lcd.fillRect(0, footerY, lcd.width(), footerHeight, bgColor);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.drawString(text, lcd.width() / 2, footerY + 7);
}

void showStatusScreen(String title, String message) {
    M5Cardputer.Display.fillScreen(0x0841);
    drawHeader(title.c_str());
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
    M5Cardputer.Display.drawString(message, 10, 30);
}

int drawWrappedText(String text, int x, int y, int maxWidth, bool alignRight, uint16_t color) {
    auto& lcd = M5.Display;
    lcd.setTextColor(color);
    lcd.setTextSize(1);

    String line = "";
    String word = "";
    int currentY = y;
    int lineHeight = 11;

    for (int i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        if (c == ' ' || i == text.length() - 1 || c == '\n') {
            if (c != ' ' && c != '\n') { word += c; }
            if (lcd.textWidth(line + word + (c == ' ' ? " " : "")) > maxWidth || c == '\n') {
                lcd.setTextDatum(alignRight ? textdatum_t::top_right : textdatum_t::top_left);
                lcd.drawString(line, alignRight ? x + maxWidth : x, currentY);
                currentY += lineHeight;
                line = (c != '\n' ? word + " " : "");
            } else {
                line += word + (c == ' ' ? " " : "");
            }
            word = "";
            if (c == '\n') line = "";
        } else {
            word += c;
        }
    }
    lcd.setTextDatum(alignRight ? textdatum_t::top_right : textdatum_t::top_left);
    lcd.drawString(line + word, alignRight ? x + maxWidth : x, currentY);

    return currentY + lineHeight - y;
}

void drawMainMenu() {
    auto& lcd = M5.Display;
    lcd.clear(0x0841);
    drawHeader("WhatsApp Client");

    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);

    String wifiStatus = WiFi.status() == WL_CONNECTED ? String(g_wifi_ssid) : "Not Connected";
    lcd.setTextColor(WHITE);
    lcd.drawString("WiFi: " + wifiStatus, 10, 28);
    lcd.drawString("Server: " + String(g_server_ip) + ":" + String(g_server_port), 10, 42);

    const char* menuItems[2] = {"Chats", "Settings"};
    for (int i = 0; i < 2; i++) {
        int y = 65 + i * 35;
        if (i == selectedMainMenu) {
            lcd.fillRoundRect(10, y, 220, 30, 4, 0x0451);
            lcd.setTextColor(WHITE);
        } else {
            lcd.fillRoundRect(10, y, 220, 30, 4, 0x1082);
            lcd.setTextColor(WHITE);
        }
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.drawString(menuItems[i], 120, y + 15);
    }

    drawFooter("UP/DOWN + ENTER");
}

void drawWifiScanPage() {
    auto& lcd = M5.Display;
    lcd.clear(0x0841);
    drawHeader("Select WiFi Network");

    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);

    int maxOnScreen = 11;
    int startY = 22;
    for (int i = 0; i < maxOnScreen && i < wifiNetworkCount; i++) {
        int y = startY + i * 18;

        if (i == selectedWifiIndex) {
            lcd.fillRoundRect(5, y - 2, 230, 16, 3, 0x0451);
            lcd.setTextColor(WHITE);
        } else {
            lcd.setTextColor(0xAD55);
        }

        String displayName = wifiNetworks[i];
        if (displayName.length() > 28) {
            displayName = displayName.substring(0, 25) + "...";
        }
        lcd.drawString(displayName, 10, y);
    }

    drawFooter("` BACK | UP/DOWN | ENTER");
}

void drawPasswordInputPage() {
    auto& lcd = M5.Display;

    // Update button state (G0)
    M5.update();
    if (M5.BtnA.wasPressed() or M5.BtnC.wasPressed()) {  // G0 button on Cardputer
        showPassword = !showPassword;
        needsRedraw = true;
    }

    if (!needsRedraw) {
        // Update only the password field and toggle button
        lcd.fillRect(5, 50, 230, 24, 0x2104);
        lcd.setCursor(10, 55);
        lcd.setTextColor(WHITE);
        lcd.setTextSize(1);

        // Create masked or visible password text
        String display;
        if (showPassword) {
            display = passwordInputBuffer;
        } else {
            for (size_t i = 0; i < passwordInputBuffer.length(); i++) {
                display += '*';
            }
        }
        lcd.print(display + "_");

        // Draw the Show/Hide button
        lcd.fillRoundRect(180, 80, 50, 16, 3, 0x0451);
        lcd.setTextDatum(textdatum_t::middle_center);
        lcd.setTextColor(WHITE);
        lcd.drawString(showPassword ? "Hide" : "Show", 205, 88);
        return;
    }

    // Full redraw
    lcd.clear(0x0841);
    drawHeader("Enter WiFi Password");

    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);
    lcd.setTextColor(0xAD55);

    // Show SSID
    String displaySSID = wifiNetworks[selectedWifiIndex];
    if (displaySSID.length() > 30) {
        displaySSID = displaySSID.substring(0, 27) + "...";
    }
    lcd.drawString("SSID: " + displaySSID, 10, 28);

    // Password input box
    lcd.fillRect(5, 50, 230, 24, 0x2104);
    lcd.setCursor(10, 55);
    lcd.setTextColor(WHITE);

    String display;
    if (showPassword) {
        display = passwordInputBuffer;
    } else {
        for (size_t i = 0; i < passwordInputBuffer.length(); i++) {
            display += '*';
        }
    }
    lcd.print(display + "_");

    // Show/Hide toggle button
    lcd.fillRoundRect(180, 80, 50, 16, 3, 0x0451);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(WHITE);
    lcd.drawString(showPassword ? "Hide" : "Show", 205, 88);

    // Footer text
    drawFooter("G0 TOGGLE | ` BACK | DEL | ENTER");

    needsRedraw = false;
}


void drawServerConfigPage() {
    auto& lcd = M5.Display;

    if (!needsRedraw) {
        lcd.fillRect(5, 60, 230, 24, 0x2104);
        lcd.setCursor(10, 65);
        lcd.setTextColor(WHITE);
        lcd.setTextSize(1);
        String display = serverInputBuffer;
        if (display.length() > 30) display = display.substring(display.length() - 30);
        lcd.print(display + "_");
        return;
    }

    lcd.clear(0x0841);

    if (serverConfigStep == 0) {
        drawHeader("Server IP Address");
        lcd.setTextSize(1);
        lcd.setTextDatum(textdatum_t::top_left);
        lcd.setTextColor(0xAD55);
        lcd.drawString("Current: " + String(g_server_ip), 10, 28);
        lcd.setTextColor(WHITE);
        lcd.drawString("New IP:", 10, 45);
    } else {
        drawHeader("Server Port");
        lcd.setTextSize(1);
        lcd.setTextDatum(textdatum_t::top_left);
        lcd.setTextColor(0xAD55);
        lcd.drawString("Current: " + String(g_server_port), 10, 28);
        lcd.setTextColor(WHITE);
        lcd.drawString("New Port:", 10, 45);
    }

    lcd.fillRect(5, 60, 230, 24, 0x2104);
    lcd.setCursor(10, 65);
    lcd.print(serverInputBuffer + "_");

    drawFooter("` BACK | DEL | ENTER");
    needsRedraw = false;
}

void drawChatsPage() {
    auto& lcd = M5.Display;
    lcd.clear(0x0841);
    drawHeader("Chats");

    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);

    int maxOnScreen = 11;
    int startY = 22;
    for (int i = 0; i < maxOnScreen; i++) {
        int chatIndex = i + chatListScrollOffset;
        if (chatIndex >= chatCount) break;

        int y = startY + i * 18;

        if (chatIndex == selectedChatIndex) {
            lcd.fillRoundRect(5, y - 2, 230, 16, 3, 0x0451);
            lcd.setTextColor(WHITE);
        } else {
            lcd.setTextColor(0xAD55);
        }

        String name = chatList[chatIndex].name;
        if (name.length() > 18) name = name.substring(0, 15) + "...";
        lcd.drawString(name, 10, y);

        String lastText = chatList[chatIndex].lastText;
        if (lastText.length() > 20) lastText = lastText.substring(0, 17) + "...";
        lcd.drawString(lastText, 100, y);

        if (chatList[chatIndex].count > 0) {
            lcd.fillCircle(210, y + 8, 6, 0xF800);
            lcd.setTextColor(WHITE);
            lcd.setTextDatum(textdatum_t::middle_center);
            lcd.drawString(String(chatList[chatIndex].count), 210, y + 8);
        }
    }

    drawFooter("` BACK | UP/DOWN | ENTER");
}

void drawChatView() {
    auto& lcd = M5.Display;
    lcd.clear(0x0841);
    drawHeader(chatList[selectedChatIndex].name.c_str());

    lcd.setTextSize(1);

    int screenWidth = 240;
    int messageMargin = 8;
    int textWidth = screenWidth - (2 * messageMargin) - 8;

    int currentY = lcd.height() - 22;

    for (int i = messageCount - 1; i >= 0; i--) {
        String text = currentMessages[i].text;
        bool isMe = (currentMessages[i].from == "me");

        int lineCount = (text.length() * 8 / textWidth) + 1;
        int textHeight = lineCount * 11;
        int messageBlockHeight = textHeight + 8;

        if (currentY - messageBlockHeight < 20) break;

        currentY -= messageBlockHeight;

        uint16_t boxColor = isMe ? 0x0451 : 0x2965;
        uint16_t textColor = WHITE;

        lcd.fillRoundRect(isMe ? screenWidth - textWidth - messageMargin - 8 : messageMargin,
                         currentY,
                         textWidth + 8,
                         messageBlockHeight,
                         4,
                         boxColor);

        drawWrappedText(text,
                        isMe ? screenWidth - textWidth - messageMargin - 4 : messageMargin + 4,
                        currentY + 4,
                        textWidth,
                        isMe,
                        textColor);

        currentY -= 5;
    }

    drawFooter("` BACK | ENTER to Type");
}

void drawInputBox() {
    auto& lcd = M5.Display;
    lcd.fillRect(0, 218, 240, 22, 0x2104);
    lcd.setCursor(5, 222);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    String display = messageInputBuffer;
    if (display.length() > 36) display = display.substring(display.length() - 36);
    lcd.print(display + "_");
}

void drawSettingsPage() {
    auto& lcd = M5.Display;
    lcd.clear(0x0841);
    drawHeader("Settings");

    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);

    const char* settings[] = {"WiFi Setup", "Server IP/Port", "Reset Config"};
    const int settingsCount = 3;

    int startY = 30;
    for (int i = 0; i < settingsCount; i++) {
        int y = startY + i * 24;
        if (i == selectedSetting) {
            lcd.fillRoundRect(10, y - 2, 220, 20, 4, 0x0451);
            lcd.setTextColor(WHITE);
        } else {
            lcd.setTextColor(0xAD55);
        }
        lcd.setTextDatum(textdatum_t::middle_left);
        lcd.drawString(settings[i], 20, y + 8);
    }

    drawFooter("` BACK | UP/DOWN | ENTER");
}
