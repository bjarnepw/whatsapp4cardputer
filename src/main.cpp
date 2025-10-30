#include <M5Cardputer.h>
#include <WiFi.h>
#include "config.h"
#include "preferences_manager.h"
#include "wifi_manager.h"
#include "ui_manager.h"
#include "input_handler.h"

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5.Display.setTextFont(&fonts::Font2);
    M5.Display.setTextSize(1);
    M5.Display.setRotation(1);

    Serial.begin(115200);

    showStatusScreen("Starting...", "Loading Configuration...");
    loadPreferences();

    if (strlen(g_wifi_ssid) > 0) {
        showStatusScreen("WiFi Connect", "Connecting to: " + String(g_wifi_ssid));
        WiFi.begin(g_wifi_ssid, g_wifi_password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            M5Cardputer.Display.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            showStatusScreen("Connected!", WiFi.localIP().toString());
            delay(1000);
        } else {
            showStatusScreen("Failed", "Could not connect. Use Settings.");
            delay(2000);
        }
    } else {
        showStatusScreen("No WiFi", "Configure WiFi in Settings");
        delay(2000);
    }

    currentState = STATE_BOOT;
    needsRedraw = true;
}

void loop() {
    M5Cardputer.update();

    if (needsRedraw) {
        switch (currentState) {
            case STATE_BOOT:
                drawMainMenu();
                break;
            case STATE_WIFI_SCAN:
                drawWifiScanPage();
                break;
            case STATE_WIFI_PASSWORD_INPUT:
                drawPasswordInputPage();
                break;
            case STATE_SERVER_CONFIG:
                drawServerConfigPage();
                break;
            case STATE_CHAT_LIST:
                drawChatsPage();
                break;
            case STATE_CHAT_VIEW:
                drawChatView();
                break;
            case STATE_TYPING_MESSAGE:
                drawChatView();
                drawInputBox();
                break;
            case STATE_SETTINGS:
                drawSettingsPage();
                break;
        }
        needsRedraw = false;
    }

    switch (currentState) {
        case STATE_BOOT:
            handleMainMenuInput();
            break;
        case STATE_WIFI_SCAN:
            handleWifiScanInput();
            break;
        case STATE_WIFI_PASSWORD_INPUT:
            handlePasswordInput();
            break;
        case STATE_SERVER_CONFIG:
            handleServerConfigInput();
            break;
        case STATE_CHAT_LIST:
            handleChatListInput();
            break;
        case STATE_CHAT_VIEW:
            handleChatViewInput();
            break;
        case STATE_TYPING_MESSAGE:
            handleTypingInput();
            break;
        case STATE_SETTINGS:
            handleSettingsInput();
            break;
    }

    delay(10);
}
