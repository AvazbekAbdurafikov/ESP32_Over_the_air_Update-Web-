#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "Your_SSID";
const char* password = "Your_Password";

// Replace with the IP address and port of your Flask server
const char* serverUrl = "Your_IP_Address:5000";
const char* updateCheckUrl = "Your_IP_Address:5000/check_for_update";

void checkForUpdate();
void performOTAUpdate(const char* updateUrl);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void checkForUpdate() {
  HTTPClient http;
  http.setTimeout(20000);  // Set timeout to 20 seconds

  Serial.println("Checking for update...");
  http.begin(updateCheckUrl);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Update check response: " + payload);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    bool updateAvailable = doc["update"];
    if (updateAvailable) {
      String filename = doc["filename"];
      String updateUrl = String(serverUrl) + "/uploads/" + filename;
      performOTAUpdate(updateUrl.c_str());
    } else {
      Serial.println("No update available.");
    }
  } else {
    Serial.println("Update check failed, error: " + String(httpCode));
  }
  http.end();
}

void performOTAUpdate(const char* updateUrl) {
  HTTPClient http;
  http.setTimeout(20000);  // Set timeout to 20 seconds

  Serial.println("HTTP GET request to: " + String(updateUrl));
  http.begin(updateUrl);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    if (!Update.begin(len)) {
      Serial.println("Update.begin() failed");
      return;
    }

    size_t written = Update.writeStream(*stream);
    if (written == len) {
      Serial.println("Written : " + String(written) + " successfully");
    } else {
      Serial.println("Written only : " + String(written) + "/" + String(len));
    }

    if (Update.end()) {
      Serial.println("Update complete");
      if (Update.isFinished()) {
        Serial.println("Update successfully finished. Rebooting.");
        ESP.restart();
      } else {
        Serial.println("Update not finished. Something went wrong.");
      }
    } else {
      Serial.println("Update failed. Error #: " + String(Update.getError()));
    }
  } else {
    Serial.println("HTTP GET request failed, error: " + String(httpCode));
  }
  http.end();
}

void loop() {
  checkForUpdate();
  delay(30000);  // Check for update every 60 seconds
}
