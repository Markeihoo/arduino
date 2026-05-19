#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT11  // หรือ DHT22

const char* ssid = "ชื่อ";
const char* password = "รหัส";

// เปลี่ยนเป็น IP เครื่อง
const char* serverUrl = "http://192.168.137.1:5678/webhook-test/iot-dht&quot;;  //local IP

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read DHT!");
      delay(5000);
      return;
    }

    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"device_id\":\"wemos-01\",";
    json += "\"temperature\":" + String(t, 1) + ",";
    json += "\"humidity\":" + String(h, 1);
    json += "}";

    Serial.println("Sending:");
    Serial.println(json);

    int httpCode = http.POST(json);

    Serial.print("HTTP Code: ");
    Serial.println(httpCode);

    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);

    http.end();

  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(10000);
}
