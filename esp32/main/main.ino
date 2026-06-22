#include "LittleFSTest.h"
#include "ota.h" // have funcs about ota
#include <WiFi.h>

// Wi-Fi Configuration
#define USE_WPA2_ENTERPRISE // Uncomment this line to use WPA2 Enterprise (e.g. NTUST-PEAP)

#ifdef USE_WPA2_ENTERPRISE
const String ssid = "NTUST-PEAP";
const String eap_identity = ""; // Normally empty or your student ID
const String eap_username = "B11230017"; // EAP username (e.g., student ID)
const String eap_password = "YOUR_PASSWORD";   // EAP password
#else
const String ssid = "sumimi";
const String password = "Q9988551";
#endif

const String server_url = "https://leepotsung.pythonanywhere.com";
const String check_path = "/api/check";

// main
void setup() {
  Serial.begin(115200);
  delay(5000);                  // I open monitor. see debug msg
  pinMode(LED_BUILTIN, OUTPUT); // LED, for test ota

  initFS();
  initOTA(server_url, check_path);
  listDir(LittleFS, "/", 1);
  // WiFi
#ifdef USE_WPA2_ENTERPRISE
  if (!initWiFiEnterprise(ssid, eap_identity, eap_username, eap_password)) {
    ESP.restart();
  }
#else
  if (!initWiFi(ssid, password)) {
    ESP.restart();
  }
#endif
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
#ifdef USE_WPA2_ENTERPRISE
    if (!initWiFiEnterprise(ssid, eap_identity, eap_username, eap_password))
      ESP.restart();
#else
    if (!initWiFi(ssid, password))
      ESP.restart();
#endif
  }

  delay(6000); // every 6s, check version (temp

  // digitalWrite(LED_BUILTIN, LOW); // test code
}
