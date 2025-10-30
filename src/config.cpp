#include "config.h"

char g_server_ip[40] = "192.168.1.100";
char g_server_port[6] = "3000";
char g_wifi_ssid[32] = "";
char g_wifi_password[64] = "";

AppState currentState = STATE_BOOT;
bool needsRedraw = true;

ChatEntry chatList[MAX_CHATS];
int chatCount = 0;
MessageEntry currentMessages[MAX_MESSAGES];
int messageCount = 0;

int chatListScrollOffset = 0;
int selectedChatIndex = 0;
int selectedWifiIndex = 0;
int wifiNetworkCount = 0;
String wifiNetworks[20];
String messageInputBuffer = "";
String passwordInputBuffer = "";
String serverInputBuffer = "";
int serverConfigStep = 0;
int selectedMainMenu = 0;
int selectedSetting = 0;

unsigned long lastKeyPress = 0;
