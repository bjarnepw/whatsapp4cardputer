#include "input_handler.h"
#include "config.h"
#include "ui_manager.h"
#include "wifi_manager.h"
#include "api_client.h"
#include "preferences_manager.h"
#include <M5Cardputer.h>
#include <WiFi.h>

bool checkKeyPress() {
    unsigned long now = millis();
    if (now - lastKeyPress > DEBOUNCE_DELAY) {
        lastKeyPress = now;
        return true;
    }
    return false;
}

void handleMainMenuInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(';') && checkKeyPress()) {
        selectedMainMenu = max(0, selectedMainMenu - 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.') && checkKeyPress()) {
        selectedMainMenu = min(1, selectedMainMenu + 1);
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        if (selectedMainMenu == 0) {
            if (WiFi.status() != WL_CONNECTED) {
                showStatusScreen("Error", "WiFi not connected! Go to Settings.");
                delay(2000);
                needsRedraw = true;
                return;
            }
            showStatusScreen("Loading Chats...", "Updating List...");
            if (fetchChatList()) {
                currentState = STATE_CHAT_LIST;
                selectedChatIndex = 0;
                chatListScrollOffset = 0;
                needsRedraw = true;
            } else {
                showStatusScreen("Error", "Could not load chats.");
                delay(2000);
                needsRedraw = true;
            }
        } else if (selectedMainMenu == 1) {
            currentState = STATE_SETTINGS;
            selectedSetting = 0;
            needsRedraw = true;
        }
    }
}

void handleWifiScanInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(';') && checkKeyPress()) {
        selectedWifiIndex = max(0, selectedWifiIndex - 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.') && checkKeyPress()) {
        selectedWifiIndex = min(wifiNetworkCount - 1, selectedWifiIndex + 1);
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        passwordInputBuffer = "";
        currentState = STATE_WIFI_PASSWORD_INPUT;
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_SETTINGS;
        needsRedraw = true;
    }
}

void handlePasswordInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    bool updated = false;

    for (auto i : keyState.word) {
        if (i >= 32 && i <= 126) {
            passwordInputBuffer += (char)i;
            updated = true;
        }
    }

    if (keyState.del && passwordInputBuffer.length() > 0) {
        passwordInputBuffer.remove(passwordInputBuffer.length() - 1);
        updated = true;
    }

    if (keyState.enter && checkKeyPress()) {
        if (connectToWifi(wifiNetworks[selectedWifiIndex], passwordInputBuffer)) {
            currentState = STATE_BOOT;
            needsRedraw = true;
        } else {
            currentState = STATE_WIFI_SCAN;
            needsRedraw = true;
        }
        return;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_WIFI_SCAN;
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawPasswordInputPage();
    }
    delay(100);
}

void handleServerConfigInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    bool updated = false;

    for (auto i : keyState.word) {
        if (i >= 32 && i <= 126) {
            serverInputBuffer += (char)i;
            updated = true;
        }
    }

    if (keyState.del && serverInputBuffer.length() > 0) {
        serverInputBuffer.remove(serverInputBuffer.length() - 1);
        updated = true;
    }

    if (keyState.enter && checkKeyPress()) {
        if (serverConfigStep == 0) {
            String newIP = serverInputBuffer;
            serverInputBuffer = "";
            serverConfigStep = 1;
            saveServerConfig(newIP, String(g_server_port));
            needsRedraw = true;
        } else {
            String newPort = serverInputBuffer;
            saveServerConfig(String(g_server_ip), newPort);
            serverInputBuffer = "";
            serverConfigStep = 0;
            showStatusScreen("Saved!", "Server: " + String(g_server_ip) + ":" + String(g_server_port));
            delay(1500);
            currentState = STATE_SETTINGS;
            needsRedraw = true;
        }
        return;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        serverInputBuffer = "";
        serverConfigStep = 0;
        currentState = STATE_SETTINGS;
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawServerConfigPage();
    }

    delay(100);
}

void handleChatListInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(';') && checkKeyPress()) {
        selectedChatIndex = max(0, selectedChatIndex - 1);
        if (selectedChatIndex < chatListScrollOffset) chatListScrollOffset = selectedChatIndex;
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.') && checkKeyPress()) {
        selectedChatIndex = min(chatCount - 1, selectedChatIndex + 1);
        if (selectedChatIndex > chatListScrollOffset + 10) chatListScrollOffset = selectedChatIndex - 10;
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        String phone = chatList[selectedChatIndex].phone;
        showStatusScreen("Loading Chat...", chatList[selectedChatIndex].name);
        if (fetchChatMessages(phone)) {
            currentState = STATE_CHAT_VIEW;
            needsRedraw = true;
        } else {
            showStatusScreen("Error", "Could not load messages.");
            delay(2000);
            needsRedraw = true;
        }
    }
    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_BOOT;
        needsRedraw = true;
    }
    delay(100);
}

void handleChatViewInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_CHAT_LIST;
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        messageInputBuffer = "";
        currentState = STATE_TYPING_MESSAGE;
        needsRedraw = true;
    }
}

void handleTypingInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    bool updated = false;

    for (auto i : keyState.word) {
        if (i >= 32 && i <= 126) {
            messageInputBuffer += (char)i;
            updated = true;
        }
    }

    if (keyState.del) {
        if (messageInputBuffer.length() > 0) {
            messageInputBuffer.remove(messageInputBuffer.length() - 1);
            updated = true;
        }
    }

    if (keyState.enter && checkKeyPress()) {
        if (messageInputBuffer.length() > 0) {
            String phone = chatList[selectedChatIndex].phone;
            showStatusScreen("Sending...", messageInputBuffer);
            if (sendMessage(phone, messageInputBuffer)) {
                fetchChatMessages(phone);
            } else {
                showStatusScreen("Error", "Send failed.");
            }
            messageInputBuffer = "";
            currentState = STATE_CHAT_VIEW;
            needsRedraw = true;
            return;
        }
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_CHAT_VIEW;
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawChatView();
        drawInputBox();
    }

    delay(100);
}

void handleSettingsInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(';') && checkKeyPress()) {
        selectedSetting = max(0, selectedSetting - 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.') && checkKeyPress()) {
        selectedSetting = min(2, selectedSetting + 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_BOOT;
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        if (selectedSetting == 0) {
            scanWifiNetworks();
        } else if (selectedSetting == 1) {
            serverInputBuffer = "";
            serverConfigStep = 0;
            currentState = STATE_SERVER_CONFIG;
            needsRedraw = true;
        } else if (selectedSetting == 2) {
            showStatusScreen("Resetting...", "Clearing all config...");
            clearPreferences();
            WiFi.disconnect();
            delay(1000);
            ESP.restart();
        }
    }
}
