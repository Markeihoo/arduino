#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>

// ตั้งค่าการเชื่อมต่อ WiFi
#define BAUD_RATE 115200
#define SSID "Markeihooooo"
#define PASSWORD "0918168125"

// ตั้งค่า MQTT
#define MQTT_PORT 1883
#define PUBLISH_INTERVAL 600000L   // ส่งข้อมูลทุก 15 นาที // สูตรคำนวณ เลขนาทีที่ต้องการ × 60 × 1000

const char* MQTT_SERVER = "mqtt.netpie.io";
const char* CLIENT_ID   = "7c124a48-41f9-420a-a1a6-273bef49291d";
const char* TOKEN       = "psbNCS12m3MtMxsHhJqQ16epqpaxjvDH";
const char* SECRET      = "JH5HTs2Q9L4zveozV6UcxTATimVyZJfn";

// หัวข้อที่ใช้ส่งข้อมูล
const char* TOPIC_DATA   = "@msg/room/data";
const char* TOPIC_STATUS = "@msg/room/status";

// ตั้งค่าเซนเซอร์ DHT
#define DHT_PIN  D4
#define DHT_TYPE DHT11   // ถ้าใช้ DHT22 ให้เปลี่ยนเป็น DHT22

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
PubSubClient client(wifiClient);

unsigned long lastPublishTime = 0;

// ประกาศฟังก์ชัน
void connectWiFi();
void connectMQTT();
void initTime();
String getFormattedTime();
void publishTempHumidityData();

void setup() {
  Serial.begin(BAUD_RATE);
  delay(100);

  dht.begin();          // เริ่มต้นเซนเซอร์
  connectWiFi();        // ต่อ WiFi
  initTime();           // ดึงเวลาจริงจาก NTP

  client.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();        // ต่อ MQTT Broker

  publishTempHumidityData();   // ส่งข้อมูลครั้งแรกทันที
  lastPublishTime = millis();
}

void loop() {
  // ถ้า WiFi หลุดให้ต่อใหม่
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    initTime();   // ต่อ WiFi ใหม่แล้ว sync เวลาอีกครั้ง
  }

  // ถ้า MQTT หลุดให้ต่อใหม่
  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();

  // ส่งข้อมูลตามช่วงเวลาที่กำหนด
  unsigned long now = millis();
  if (now - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = now;
    publishTempHumidityData();
  }
}

void connectWiFi() {
  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT Broker...");

    if (client.connect(CLIENT_ID, TOKEN, SECRET)) {
      Serial.println("Connected to MQTT Broker!");
      client.publish(TOPIC_STATUS, "Witty Cloud connected");
    } else {
      Serial.print("Failed, state = ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void initTime() {
  Serial.println("Syncing time with NTP...");

  // ตั้งเวลาไทย UTC+7
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  int retry = 0;

  // รอจนกว่าจะได้เวลาจริง
  while (now < 100000 && retry < 30) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retry++;
  }

  if (now >= 100000) {
    Serial.println("\nTime synchronized");
  } else {
    Serial.println("\nFailed to sync time");
  }
}

String getFormattedTime() {
  // อ่านเวลาปัจจุบันแล้วแปลงเป็นข้อความ
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  if (!timeinfo) {
    return "1970-01-01 00:00:00";
  }

  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

void publishTempHumidityData() {
  // อ่านค่าจากเซนเซอร์
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float tempF = dht.readTemperature(true);

  // อ่านเวลาปัจจุบัน
  String timestamp = getFormattedTime();

  // ถ้าอ่านค่าไม่ได้ ให้หยุด
  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // สร้างข้อมูล JSON ที่จะส่งออก
  String payload = "{";
  payload += "\"device_id\":\"witty01\",";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"temperature_c\":" + String(tempC, 2) + ",";
  payload += "\"temperature_f\":" + String(tempF, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2);
  payload += "}";

  // ส่งข้อมูลไปยัง MQTT
  bool ok = client.publish(TOPIC_DATA, payload.c_str());

  if (ok) {
    Serial.println("Published: " + payload);
  } else {
    Serial.println("Publish failed");
  }
}
