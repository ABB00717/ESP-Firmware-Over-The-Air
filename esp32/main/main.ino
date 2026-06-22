#include "LittleFSTest.h"
#include "ota.h" // have funcs about ota
#include <WiFi.h>

// Dynamic Wi-Fi & Server config variables loaded from LittleFS config.json on boot
String wifi_ssid;
String wifi_password;
String eap_identity;
String eap_username;
bool use_enterprise = false;
extern String server_url;
const String check_path = "/api/check";

// main
void setup() {
  Serial.begin(115200);
  delay(5000);                  // I open monitor. see debug msg
  pinMode(LED_BUILTIN, OUTPUT); // LED, for test ota

  // Initialize Filesystem first
  if (!initFS()) {
    Serial.println("Critical error: LittleFS initialization failed. Rebooting...");
    delay(1000);
    ESP.restart();
  }

  // Load configs dynamically
  if (!loadConfig(wifi_ssid, wifi_password, eap_identity, eap_username, use_enterprise, server_url)) {
    Serial.println("[Config] /config.json is missing or invalid!");
    Serial.println("[Config] Please paste your config JSON (one line) over Serial within 15 seconds...");
    uint32_t start_wait = millis();
    String serial_input = "";
    while (millis() - start_wait < 15000) {
      if (Serial.available()) {
        char c = Serial.read();
        serial_input += c;
        if (c == '\n') break;
      }
      delay(5);
    }
    serial_input.trim();
    if (serial_input.startsWith("{") && serial_input.endsWith("}")) {
      File file = LittleFS.open("/config.json", "w");
      if (file) {
        file.print(serial_input);
        file.close();
        Serial.println("[Config] Configuration saved! Rebooting...");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("[Config] Failed to open /config.json for writing!");
      }
    }
    Serial.println("[Config] No valid configuration received. Rebooting...");
    delay(2000);
    ESP.restart();
  }

  initOTA(server_url, check_path);
  listDir(LittleFS, "/", 1);

  // WiFi Connection
  bool connected = false;
  if (use_enterprise) {
    connected = initWiFiEnterprise(wifi_ssid, eap_identity, eap_username, wifi_password);
  } else {
    connected = initWiFi(wifi_ssid, wifi_password);
  }

  if (!connected) {
    ESP.restart();
  }

  markFirmwareValid();
}

void loop() {
  // digitalWrite(LED_BUILTIN, HIGH); // test code

  // if wifi connected then check the latest firmware
  if (WiFi.status() == WL_CONNECTED) {
    // if the version greater than esp32 version then ota
    if (check()) {
      int count = 0;
      while (!downloadFirmwareToFS()) {
        count++;
        if (count == 3) {
          ESP.restart();
        }
      }
      OTA();
    }
  } else {
    // if cannot reconnect then restart esp32
    bool reconnected = false;
    if (use_enterprise) {
      reconnected = initWiFiEnterprise(wifi_ssid, eap_identity, eap_username, wifi_password);
    } else {
      reconnected = initWiFi(wifi_ssid, wifi_password);
    }
    if (!reconnected) {
      ESP.restart();
    }
  }

  delay(6000); // every 6s, check version (temp

  // digitalWrite(LED_BUILTIN, LOW); // test code
}
