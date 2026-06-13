#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <Arduino.h>

bool checkKeyPress();

void handleMainMenuInput();
void handleWifiScanInput();
void handlePasswordInput();
void handleServerConfigInput();
void handleChatListInput();
void handleChatViewInput();
void handleTypingInput();
void handleSettingsInput();
void handleNgrokChoiceInput();
void handleNgrokUsernameInput();
void handleNgrokPasswordInput();
void handleServerAddressInput();
void handleServerPortInput();

#endif
