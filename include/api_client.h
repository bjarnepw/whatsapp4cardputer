#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>

String buildApiUrl(String path);
bool fetchChatList();
bool fetchChatMessages(String phone);
bool sendMessage(String phone, String text);

#endif
