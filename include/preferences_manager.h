#ifndef PREFERENCES_MANAGER_H
#define PREFERENCES_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

extern Preferences preferences;

void loadPreferences();
void saveWifiCredentials(String ssid, String password);
void saveServerConfig(String ip, String port);
void saveNgrokConfig(bool useNgrok, String username, String password);
void clearPreferences();

#endif
