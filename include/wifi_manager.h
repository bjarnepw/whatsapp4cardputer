#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

void scanWifiNetworks();
bool connectToWifi(String ssid, String password);

#endif
