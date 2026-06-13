#include "wifi_manager.h"
#include "config.h"
#include "ui_manager.h"
#include "preferences_manager.h"
#include <WiFi.h>
#include <M5Cardputer.h>

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
