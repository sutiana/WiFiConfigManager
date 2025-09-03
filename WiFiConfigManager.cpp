#include <WiFi.h>
#include "WiFiConfigManager.h"

// --- HTML page updated with Radio Buttons, JavaScript, and English text ---
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP32 WiFi Config</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4;}
  .container{background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);}
  h1,h2{color: #333; text-align: center;}
  label{font-weight: bold;}
  input[type=text], input[type=password]{width: 100%; padding: 12px; margin: 8px 0; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box;}
  button{background-color: #007bff; color: white; padding: 14px 20px; margin: 15px 0; border: none; border-radius: 4px; cursor: pointer; width: 100%; font-size: 16px;}
  button:hover{background-color: #0056b3;}
  .radio-group{margin: 10px 0 20px 0;}
  .radio-group label{margin-right: 15px;}
</style>
<script>
  function toggleFields() {
    var mode = document.querySelector('input[name="mode"]:checked').value;
    document.getElementById('sta_fields').style.display = (mode == '0') ? 'block' : 'none';
    document.getElementById('ap_fields').style.display = (mode == '1') ? 'block' : 'none';
  }
</script>
</head>
<body onload="toggleFields()">
<div class="container">
  <h1>ESP32 WiFi Configuration</h1>
  <form action="/save" method="POST">
    <h2>Select Operating Mode</h2>
    <div class="radio-group">
      <label><input type="radio" name="mode" value="0" onchange="toggleFields()" checked> Station (Connect to an existing WiFi)</label>
      <label><input type="radio" name="mode" value="1" onchange="toggleFields()"> Access Point (Create its own WiFi)</label>
    </div>

    <div id="sta_fields">
      <h3>Station Mode</h3>
      <label for="sta_ssid">WiFi Name (SSID)</label>
      <input type="text" id="sta_ssid" name="sta_ssid" placeholder="Your home WiFi name...">
      <label for="sta_pass">WiFi Password</label>
      <input type="password" id="sta_pass" name="sta_pass" placeholder="Your home WiFi password...">
    </div>

    <div id="ap_fields" style="display:none;">
      <h3>Access Point Mode</h3>
      <label for="ap_ssid">New AP Name</label>
      <input type="text" id="ap_ssid" name="ap_ssid" placeholder="Example: ESP32_Sensor">
      <label for="ap_pass">New AP Password (min. 8 characters)</label>
      <input type="password" id="ap_pass" name="ap_pass" placeholder="Password for the new AP">
    </div>
    
    <button type="submit">Save & Restart</button>
  </form>
</div>
</body></html>
)rawliteral";

WiFiConfigManager::WiFiConfigManager() {}

void WiFiConfigManager::beginAndRunInBackground(int core) {
  _preferences.begin(_namespace, false);
  xTaskCreatePinnedToCore(this->taskRunner, "WiFiMgrTask", 8192, this, 1, NULL, core);
}

void WiFiConfigManager::taskRunner(void* parameter) {
  static_cast<WiFiConfigManager*>(parameter)->portalLoop();
}

// Main booting logic is now here
void WiFiConfigManager::portalLoop() {
  // Check the saved mode on boot
  int savedMode = _preferences.getInt(_modeKey, -1); // -1 if never saved

  if (savedMode == MODE_STA) {
    String ssid = _preferences.getString(_staSsidKey, "");
    if (ssid.length() > 0) {
      WiFi.begin(ssid.c_str(), _preferences.getString(_staPassKey, "").c_str());
      Serial.print("Station Mode. Connecting to: ");
      Serial.println(ssid);
      int retries = 0;
      while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("\nFailed to connect. Starting configuration portal.");
        startConfigPortal("ESP32-Setup");
      }
    } else {
       Serial.println("Station Mode selected but no credentials found. Starting portal.");
       startConfigPortal("ESP32-Setup");
    }
  } else if (savedMode == MODE_AP) {
      String ap_ssid = _preferences.getString(_apSsidKey, "ESP32-AP");
      String ap_pass = _preferences.getString(_apPassKey, "12345678");
      Serial.print("Access Point Mode. Creating network: ");
      Serial.println(ap_ssid);
      WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
  } else {
    // If no mode is saved, start the portal
    Serial.println("No configuration found. Starting portal.");
    startConfigPortal("ESP32-Setup");
  }

  // Loop to keep the portal running if it's active
  for (;;) {
    if (_portalRunning) {
      _dnsServer->processNextRequest();
      _server->handleClient();
    }
    vTaskDelay(10);
  }
}

// handleSave function updated to save the mode and appropriate credentials
void WiFiConfigManager::handleSave() {
  int mode = _server->arg("mode").toInt();
  _preferences.putInt(_modeKey, mode);

  if (mode == MODE_STA) {
    String sta_ssid = _server->arg("sta_ssid");
    String sta_pass = _server->arg("sta_pass");
    _preferences.putString(_staSsidKey, sta_ssid);
    _preferences.putString(_staPassKey, sta_pass);
    Serial.println("Saving Station configuration.");
  } else if (mode == MODE_AP) {
    String ap_ssid = _server->arg("ap_ssid");
    String ap_pass = _server->arg("ap_pass");
    _preferences.putString(_apSsidKey, ap_ssid);
    _preferences.putString(_apPassKey, ap_pass);
    Serial.println("Saving Access Point configuration.");
  }
  
  String html = "<html><body><h1>Configuration Saved!</h1><p>The ESP32 will restart to apply the changes.</p></body></html>";
  _server->send(200, "text/html", html);
  delay(3000);
  ESP.restart();
}

void WiFiConfigManager::clearAllCredentials(){
  _preferences.clear();
  Serial.println("All credentials and mode have been cleared.");
}

// ... startConfigPortal and handleRoot functions remain the same ...
void WiFiConfigManager::startConfigPortal(const char* apSsid) {
  _portalRunning = true;
  _server = new WebServer(80);
  _dnsServer = new DNSServer();
  WiFi.softAP(apSsid);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address for config: ");
  Serial.println(apIP);
  _dnsServer->start(53, "*", apIP);
  _server->on("/", std::bind(&WiFiConfigManager::handleRoot, this));
  _server->on("/save", std::bind(&WiFiConfigManager::handleSave, this));
  _server->onNotFound(std::bind(&WiFiConfigManager::handleRoot, this));
  _server->begin();
}
void WiFiConfigManager::handleRoot() {
  _server->send_P(200, "text/html", HTML_PAGE);
}

void WiFiConfigManager::startConfigPortalBlocking() {
  Serial.println("Manual trigger received. Starting configuration portal...");
  
  // Stop the current WiFi mode
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);

  // Call the internal function to start the server
  startConfigPortal("ESP32-Reconfig");

  // Loop here to keep the server running.
  // This will 'block' the main loop until the ESP restarts from the web page.
  while(true) {
    _dnsServer->processNextRequest();
    _server->handleClient();
    vTaskDelay(10); // A small delay to prevent crashing
  }
}