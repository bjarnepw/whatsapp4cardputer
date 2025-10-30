#include "preferences_manager.h"
#include "config.h"

Preferences preferences;

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

