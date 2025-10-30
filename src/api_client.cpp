#include "api_client.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
    doc["to"] = phone;
    doc["text"] = text;
    String requestBody;
    serializeJson(doc, requestBody);
    int httpCode = http.POST(requestBody);
    http.end();
    return httpCode == HTTP_CODE_OK;
}
