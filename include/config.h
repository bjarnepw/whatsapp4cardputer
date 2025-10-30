#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define MAX_CHATS 50
#define MAX_MESSAGES 200
#define DEBOUNCE_DELAY 200

extern char g_server_ip[40];
extern char g_server_port[6];
extern char g_wifi_ssid[32];
extern char g_wifi_password[64];

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

extern AppState currentState;
extern bool needsRedraw;

extern ChatEntry chatList[MAX_CHATS];
extern int chatCount;
extern MessageEntry currentMessages[MAX_MESSAGES];
extern int messageCount;

extern int chatListScrollOffset;
extern int selectedChatIndex;
extern int selectedWifiIndex;
extern int wifiNetworkCount;
extern String wifiNetworks[20];
extern String messageInputBuffer;
extern String passwordInputBuffer;
extern String serverInputBuffer;
extern int serverConfigStep;
extern int selectedMainMenu;
extern int selectedSetting;

extern unsigned long lastKeyPress;

#endif
