#include "preferences_manager.h"
#include "config.h"

Preferences preferences;

void loadPreferences() {
    preferences.begin("whatsapp", false);
    preferences.getString("ssid", g_wifi_ssid, sizeof(g_wifi_ssid));
    preferences.getString("password", g_wifi_password, sizeof(g_wifi_password));
    preferences.getString("server_ip", g_server_ip, sizeof(g_server_ip));
    preferences.getString("server_port", g_server_port, sizeof(g_server_port));
    g_use_ngrok = preferences.getBool("use_ngrok", false);
    preferences.getString("ngrok_user", g_ngrok_username, sizeof(g_ngrok_username));
    preferences.getString("ngrok_pass", g_ngrok_password, sizeof(g_ngrok_password));
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

void saveNgrokConfig(bool useNgrok, String username, String password) {
    preferences.begin("whatsapp", false);
    preferences.putBool("use_ngrok", useNgrok);
    preferences.putString("ngrok_user", username);
    preferences.putString("ngrok_pass", password);
    preferences.end();
    g_use_ngrok = useNgrok;
    username.toCharArray(g_ngrok_username, sizeof(g_ngrok_username));
    password.toCharArray(g_ngrok_password, sizeof(g_ngrok_password));
}

void clearPreferences() {
    preferences.begin("whatsapp", false);
    preferences.clear();
    preferences.end();
}

