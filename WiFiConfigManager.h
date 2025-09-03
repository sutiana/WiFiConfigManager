#ifndef WiFiConfigManager_h
#define WiFiConfigManager_h

#include "Arduino.h"
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

// Enum to simplify mode management
enum WiFiMode {
  MODE_STA,
  MODE_AP
};

class WiFiConfigManager {
public:
  WiFiConfigManager();
  void beginAndRunInBackground(int core = 0);

  // Utility functions
  void clearAllCredentials();
  void startConfigPortalBlocking();

private:
  void startConfigPortal(const char* apSsid);
  void handleRoot();
  void handleSave();
  void portalLoop();
  static void taskRunner(void* parameter);

  Preferences _preferences;
  WebServer* _server = nullptr;
  DNSServer* _dnsServer = nullptr;

  // Keys for storing mode and credentials in Preferences
  const char* _namespace = "wifi-config";
  const char* _modeKey = "wifi_mode";
  const char* _staSsidKey = "sta_ssid";
  const char* _staPassKey = "sta_pass";
  const char* _apSsidKey = "ap_ssid";
  const char* _apPassKey = "ap_pass";

  bool _portalRunning = false;
};

#endif