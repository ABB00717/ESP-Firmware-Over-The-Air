// include http lib
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
// include sd card lib
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// gargs
const String ssid = "wifi";
const String password = "password";
const String firmware_url = "https://example.com/"; 

const int TRIGGER_PIN = 0; // BOOT btn

// functions
bool initWiFi() {
  WiFi.begin(ssid, password);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10) {
    delay(1000);
    Serial.print(".");
    ++count;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connect failed");
    return false;
  }
  Serial.println("\nWiFi connected");
  return true;
}

bool initSDCard() {
  Serial.print("init sd card ... ");
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("err");
    return false;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("err");
    return false;
  }

  Serial.printf("%llu MB\n", SD.cardSize() / (1024 * 1024));
  return true;
}

void startOTAUpdateHTTP(String url) {
  WiFiClient client;

  httpUpdate.onStart([]() { Serial.println("started"); });
  httpUpdate.onEnd([]() { Serial.println("ended"); });
  httpUpdate.onProgress([](int cur, int total) { Serial.printf("progress: %d / %d bytes (%.1f%%)\r", cur, total, (cur*100.0)/total); });
  httpUpdate.onError([](int err) { Serial.printf("error: %d\n", err); });
  t_httpUpdate_return ret = httpUpdate.update(client, url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("沒有新版本");
      break;
  }
}

void startOTAUpdateHTTPS(String url) {
  WiFiClientSecure client;
  client.setInsecure(); 

  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.onStart([]() { Serial.println("started"); });
  httpUpdate.onEnd([]() { Serial.println("ended"); });
  httpUpdate.onProgress([](int cur, int total) { Serial.printf("progress: %d / %d bytes (%.1f%%)\r", cur, total, (cur*100.0)/total); });
  httpUpdate.onError([](int err) { Serial.printf("error: %d\n", err); });
  t_httpUpdate_return ret = httpUpdate.update(client, url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("沒有新版本");
      break;
  }
}

// main
void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  if (!initWiFi()) {
    ESP.restart();
  }
}

void loop() {
  if (digitalRead(TRIGGER_PIN) == LOW) {
    delay(1000);
    if (firmware_url.startsWith("https://"))
      startOTAUpdateHTTPS(firmware_url);
    else if (firmware_url.startsWith("http://")) 
      startOTAUpdateHTTP(firmware_url); 
  }
}
