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
    currentState = STATE_NGROK_CHOICE;
    selectedNgrokChoice = g_use_ngrok ? 0 : 1;
    needsRedraw = true;
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
        return;
    }
    if (keyState.enter && checkKeyPress()) {
        messageInputBuffer = "";
        currentState = STATE_TYPING_MESSAGE;
        drawChatView();
        drawInputBox();
        needsRedraw = false;
        return;
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
            handleServerConfigInput();
        } else if (selectedSetting == 2) {
            showStatusScreen("Resetting...", "Clearing all config...");
            clearPreferences();
            WiFi.disconnect();
            delay(1000);
            ESP.restart();
        }
    }
}

void handleNgrokChoiceInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(';') && checkKeyPress()) {
        selectedNgrokChoice = max(0, selectedNgrokChoice - 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.') && checkKeyPress()) {
        selectedNgrokChoice = min(1, selectedNgrokChoice + 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_SETTINGS;
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        if (selectedNgrokChoice == 0) {
            ngrokUsernameBuffer = String(g_ngrok_username);
            currentState = STATE_NGROK_USERNAME_INPUT;
            needsRedraw = true;
        } else {
            serverInputBuffer = String(g_server_ip);
            currentState = STATE_SERVER_ADDRESS_INPUT;
            needsRedraw = true;
        }
    }
}

void handleNgrokUsernameInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    bool updated = false;

    for (auto i : keyState.word) {
        if (i >= 32 && i <= 126) {
            ngrokUsernameBuffer += (char)i;
            updated = true;
        }
    }

    if (keyState.del && ngrokUsernameBuffer.length() > 0) {
        ngrokUsernameBuffer.remove(ngrokUsernameBuffer.length() - 1);
        updated = true;
    }

    if (keyState.enter && checkKeyPress()) {
        ngrokPasswordBuffer = String(g_ngrok_password);
        currentState = STATE_NGROK_PASSWORD_INPUT;
        needsRedraw = true;
        return;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_NGROK_CHOICE;
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawNgrokUsernameInputPage();
    }

    delay(100);
}

void handleNgrokPasswordInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    bool updated = false;

    for (auto i : keyState.word) {
        if (i >= 32 && i <= 126) {
            ngrokPasswordBuffer += (char)i;
            updated = true;
        }
    }

    if (keyState.del && ngrokPasswordBuffer.length() > 0) {
        ngrokPasswordBuffer.remove(ngrokPasswordBuffer.length() - 1);
        updated = true;
    }

    if (keyState.enter && checkKeyPress()) {
        saveNgrokConfig(true, ngrokUsernameBuffer, ngrokPasswordBuffer);
        serverInputBuffer = String(g_server_ip);
        currentState = STATE_SERVER_ADDRESS_INPUT;
        needsRedraw = true;
        return;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_NGROK_USERNAME_INPUT;
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawNgrokPasswordInputPage();
    }

    delay(100);
}

void handleServerAddressInput() {
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
        String newIP = serverInputBuffer;
        saveServerConfig(newIP, String(g_server_port));
        serverInputBuffer = String(g_server_port);
        currentState = STATE_SERVER_PORT_INPUT;
        needsRedraw = true;
        return;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        if (selectedNgrokChoice == 0) {
            currentState = STATE_NGROK_PASSWORD_INPUT;
        } else {
            currentState = STATE_NGROK_CHOICE;
        }
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawServerAddressInputPage();
    }

    delay(100);
}

void handleServerPortInput() {
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
        String newPort = serverInputBuffer;
        saveServerConfig(String(g_server_ip), newPort);
        if (selectedNgrokChoice == 1) {
            saveNgrokConfig(false, "", "");
        }
        serverInputBuffer = "";
        showStatusScreen("Saved!", "Server: " + String(g_server_ip) + ":" + String(g_server_port));
        delay(1500);
        currentState = STATE_SETTINGS;
        needsRedraw = true;
        return;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`') && checkKeyPress()) {
        currentState = STATE_SERVER_ADDRESS_INPUT;
        needsRedraw = true;
        return;
    }

    if (updated) {
        drawServerPortInputPage();
    }

    delay(100);
}
