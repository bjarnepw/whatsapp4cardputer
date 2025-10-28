#include <M5Cardputer.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// -----------------------------------------------------------------------------
// GLOBAL VARIABLES & SETTINGS
// -----------------------------------------------------------------------------

Preferences preferences;

char g_server_ip[40] = "192.168.1.100";
char g_server_port[6] = "3000";
char g_wifi_ssid[32] = "";
char g_wifi_password[64] = "";

struct MessageEntry {
    String from;
    String text;
    long time;
};

struct ChatEntry {
    String phone;
    String name;
    String lastText;
    long lastTime;
    long count;
};

#define MAX_CHATS 5
#define MAX_MESSAGES 10
ChatEntry chatList[MAX_CHATS];
int chatCount = 0;
MessageEntry currentMessages[MAX_MESSAGES];
int messageCount = 0;

enum AppState {
    STATE_BOOT,
    STATE_WIFI_SCAN,
    STATE_WIFI_PASSWORD_INPUT,
    STATE_CHAT_LIST,
    STATE_CHAT_VIEW,
    STATE_TYPING_MESSAGE,
    STATE_SETTINGS,
    STATE_SERVER_CONFIG
};
AppState currentState = STATE_BOOT;

int chatListScrollOffset = 0;
int selectedChatIndex = 0;
int selectedWifiIndex = 0;
int wifiNetworkCount = 0;
String wifiNetworks[20];
String messageInputBuffer = "";
String passwordInputBuffer = "";
String serverInputBuffer = "";
int serverConfigStep = 0;

const char* settings[] = {"WiFi Setup", "Server IP/Port", "Reset Config"};
const int settingsCount = 3;
int selectedSetting = 0;

unsigned long lastKeyPress = 0;
const unsigned long debounceDelay = 200;

bool needsRedraw = true;

// -----------------------------------------------------------------------------
// PREFERENCES MANAGEMENT
// -----------------------------------------------------------------------------

void loadPreferences() {
    preferences.begin("whatsapp", false);
    preferences.getString("ssid", g_wifi_ssid, sizeof(g_wifi_ssid));
    preferences.getString("password", g_wifi_password, sizeof(g_wifi_password));
    preferences.getString("server_ip", g_server_ip, sizeof(g_server_ip));
    preferences.getString("server_port", g_server_port, sizeof(g_server_port));
    preferences.end();
}

void saveWifiCredentials(String ssid, String password) {
    preferences.begin("whatsapp", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    ssid.toCharArray(g_wifi_ssid, sizeof(g_wifi_ssid));
    password.toCharArray(g_wifi_password, sizeof(g_wifi_password));
}

void saveServerConfig(String ip, String port) {
    preferences.begin("whatsapp", false);
    preferences.putString("server_ip", ip);
    preferences.putString("server_port", port);
    preferences.end();
    ip.toCharArray(g_server_ip, sizeof(g_server_ip));
    port.toCharArray(g_server_port, sizeof(g_server_port));
}

void clearPreferences() {
    preferences.begin("whatsapp", false);
    preferences.clear();
    preferences.end();
}

// -----------------------------------------------------------------------------
// DEBOUNCE HELPER
// -----------------------------------------------------------------------------

bool checkKeyPress() {
    unsigned long now = millis();
    if (now - lastKeyPress > debounceDelay) {
        lastKeyPress = now;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// HELPER FUNCTIONS: UI & LOGIC
// -----------------------------------------------------------------------------

void drawHeader(const char* title, uint16_t bgColor = 0x0451) {
    auto& lcd = M5.Display;
    lcd.fillRect(0, 0, lcd.width(), 18, bgColor);
    lcd.setTextColor(WHITE);
    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.drawString(title, lcd.width() / 2, 9);
}

void drawFooter(const char* text, uint16_t bgColor = 0x2104) {
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

int drawWrappedText(String text, int x, int y, int maxWidth, bool alignRight = false, uint16_t color = WHITE) {
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

// -----------------------------------------------------------------------------
// WIFI FUNCTIONS
// -----------------------------------------------------------------------------

void scanWifiNetworks() {
    showStatusScreen("WiFi Scan", "Scanning networks...");
    wifiNetworkCount = WiFi.scanNetworks();

    if (wifiNetworkCount == 0) {
        wifiNetworkCount = 0;
        showStatusScreen("WiFi Scan", "No networks found!");
        delay(2000);
        currentState = STATE_BOOT;
        needsRedraw = true;
        return;
    }

    for (int i = 0; i < wifiNetworkCount && i < 20; i++) {
        wifiNetworks[i] = WiFi.SSID(i);
    }

    selectedWifiIndex = 0;
    currentState = STATE_WIFI_SCAN;
    needsRedraw = true;
}

bool connectToWifi(String ssid, String password) {
    showStatusScreen("WiFi Connect", "Connecting to:");
    M5Cardputer.Display.drawString(ssid, 10, 50);

    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        M5Cardputer.Display.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        showStatusScreen("Connected!", WiFi.localIP().toString());
        saveWifiCredentials(ssid, password);
        delay(1500);
        return true;
    } else {
        showStatusScreen("Failed", "Could not connect to WiFi");
        delay(2000);
        return false;
    }
}

// -----------------------------------------------------------------------------
// API FUNCTIONS
// -----------------------------------------------------------------------------

String buildApiUrl(String path) {
    return "http://" + String(g_server_ip) + ":" + String(g_server_port) + path;
}

bool fetchChatList() {
    HTTPClient http;
    String url = buildApiUrl("/chats");
    if (!http.begin(url)) return false;
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<32768> doc;

        // FIXED: Only deserialize once!
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.print("JSON Error: ");
            Serial.println(err.f_str());
            http.end();
            return false;
        }

        JsonArray array = doc.as<JsonArray>();
        chatCount = 0;
        for (JsonVariant v : array) {
            if (chatCount >= MAX_CHATS) break;
            chatList[chatCount].phone = v["phone"].as<String>();
            chatList[chatCount].name = v["name"].as<String>();
            chatList[chatCount].lastText = v["lastText"].as<String>();
            chatList[chatCount].lastTime = v["lastTime"].as<long>();
            chatList[chatCount].count = v["count"].as<long>();
            chatCount++;
        }
        http.end();
        return true;
    }
    http.end();
    return false;
}

bool fetchChatMessages(String phone) {
    HTTPClient http;
    String url = buildApiUrl("/chat/" + phone);
    if (!http.begin(url)) return false;
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<16384> doc;

        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.print("JSON Error: ");
            Serial.println(err.f_str());
            http.end();
            return false;
        }

        JsonArray array = doc.as<JsonArray>();
        messageCount = 0;
        for (JsonVariant v : array) {
            if (messageCount >= MAX_MESSAGES) break;
            currentMessages[messageCount].from = v["from"].as<String>();
            currentMessages[messageCount].text = v["text"].as<String>();
            currentMessages[messageCount].time = v["time"].as<long>();
            messageCount++;
        }
        http.end();
        return true;
    }
    http.end();
    return false;
}

bool sendMessage(String phone, String text) {
    HTTPClient http;
    String url = buildApiUrl("/send");
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<256> doc;
    doc["to"] = phone;  // Server now handles phone format correctly
    doc["text"] = text;
    String requestBody;
    serializeJson(doc, requestBody);
    int httpCode = http.POST(requestBody);
    http.end();
    return httpCode == HTTP_CODE_OK;
}

// -----------------------------------------------------------------------------
// UI DRAWING FUNCTIONS
// -----------------------------------------------------------------------------

int selectedMainMenu = 0;
const int mainMenuCount = 2; // Chats and Settings

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

    // Draw menu items with selection highlight
    const char* menuItems[mainMenuCount] = {"Chats", "Settings"};
    for (int i = 0; i < mainMenuCount; i++) {
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
    if (!needsRedraw) {
        lcd.fillRect(5, 50, 230, 24, 0x2104);
        lcd.setCursor(10, 55);
        lcd.setTextColor(WHITE);
        lcd.setTextSize(1);
        String display = passwordInputBuffer;
        for (int i = 0; i < display.length(); i++) {
            lcd.print("*");
        }
        lcd.print("_");
        return;
    }

    lcd.clear(0x0841);
    drawHeader("Enter WiFi Password");

    lcd.setTextSize(1);
    lcd.setTextDatum(textdatum_t::top_left);
    lcd.setTextColor(0xAD55);

    String displaySSID = wifiNetworks[selectedWifiIndex];
    if (displaySSID.length() > 30) {
        displaySSID = displaySSID.substring(0, 27) + "...";
    }
    lcd.drawString("SSID: " + displaySSID, 10, 28);

    lcd.fillRect(5, 50, 230, 24, 0x2104);
    lcd.setCursor(10, 55);
    lcd.setTextColor(WHITE);
    for (int i = 0; i < passwordInputBuffer.length(); i++) {
        lcd.print("*");
    }
    lcd.print("_");

    drawFooter("` BACK | DEL | ENTER");
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

        // Highlight selected chat
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
            lcd.fillCircle(210, y + 8, 6, 0xF800); // red dot
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

// -----------------------------------------------------------------------------
// INPUT HANDLERS
// -----------------------------------------------------------------------------

void handleMainMenuInput() {
    M5Cardputer.update();
    Keyboard_Class::KeysState keyState = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.Keyboard.isKeyPressed(';') && checkKeyPress()) {
        selectedMainMenu = max(0, selectedMainMenu - 1);
        needsRedraw = true;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('.') && checkKeyPress()) {
        selectedMainMenu = min(mainMenuCount - 1, selectedMainMenu + 1);
        needsRedraw = true;
    }
    if (keyState.enter && checkKeyPress()) {
        if (selectedMainMenu == 0) { // Chats
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
        } else if (selectedMainMenu == 1) { // Settings
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
        selectedSetting = min(settingsCount - 1, selectedSetting + 1);
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

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// LOOP
// -----------------------------------------------------------------------------

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
