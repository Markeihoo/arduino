#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define BAUD_RATE 115200
#define SSID "tongyu"
#define PASSWORD "btyprrr2548"
#define MQTT_PORT 1883

const char * MQTT_SERVER = "mqtt.netpie.io";
// const char * CLIENT_ID = "1e61e08d-beb5-42ac-a490-338f1fb744d8";
// const char * TOKEN = "cpTfYrBwF56sWtScdmKH5wDDRf37WdKF";
// const char * SECRET = "hEsi4wwMzEqo3GQb23bVHvHCqb9ZnbYk";
//
const char * CLIENT_ID = "7567a186-9540-425b-af9a-5dedb511cfff";
const char * TOKEN = "WnpEU2PrsrqpJgpMPYeg7XHfhxFZpME2";
const char * SECRET = "mzPfPPQw1mtnYkwkopKiMEcd5qtpa5Rk";

// Topic สำหรับ subscribe (รับข้อมูล) และ publish (ส่งข้อมูล)
const char * TOPIC_SUB = "@msg/test_a";   // รับข้อมูลจาก server
const char * TOPIC_PUB = "@msg/test_b";   // ส่งข้อมูลไป server

unsigned long lastPublishTime = 0;
const long PUBLISH_INTERVAL = 5000; // ส่งข้อมูลทุก 5 วินาที

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// ต้องประกาศฟังก์ชัน callback ไว้ข้างบน หรือสร้าง Prototype ไว้ก่อน setup
void callback(char * topic, byte * payload, unsigned int length) {
  char * msg = (char *) malloc (length + 1);
  memcpy(msg, payload, length); // แก้จาก memcpt เป็น memcpy
  msg[length] = '\0'; // แก้จาก "\0" (string) เป็น '\0' (char)
  
  String str = String(msg); // แก้จาก srt เป็น str
  Serial.printf("Message received [%s]: %s\n", topic, str.c_str());
  free(msg);
}

void setup() {
  Serial.begin(BAUD_RATE);
  
  // เชื่อมต่อ WiFi
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println("\nWiFi connected"); // แก้จาก /n เป็น \n
  Serial.print("WiFi Address: ");
  Serial.println(WiFi.localIP()); // แก้จาก Serrial เป็น Serial

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // การเชื่อมต่อ MQTT
  while (!client.connected()) {
    Serial.println("Connecting to MQTT Broker....");
    
    // สำหรับ NETPIE: ใช้ CLIENT_ID, TOKEN เป็น Username และ SECRET เป็น Password
    if (client.connect(CLIENT_ID, TOKEN, SECRET)) { 
      Serial.println("Connected to MQTT Broker!");
      client.subscribe(TOPIC_SUB); // subscribe เพื่อรับข้อมูลจาก server
    } else {
      Serial.print("Failed to connect, state: ");
      Serial.println(client.state()); // แก้จาก pront เป็น print
      delay(2000);
    }
  }
}

void loop() {
  // reconnect ถ้าหลุดการเชื่อมต่อ
  if (!client.connected()) {
    Serial.println("Reconnecting to MQTT Broker...");
    if (client.connect(CLIENT_ID, TOKEN, SECRET)) {
      Serial.println("Reconnected!");
      client.subscribe(TOPIC_SUB);
    } else {
      Serial.print("Failed, state: ");
      Serial.println(client.state());
      delay(2000);
      return;
    }
  }

  // สำคัญมาก: ต้องมี client.loop() เพื่อให้ MQTT ทำงานได้ต่อเนื่อง
  client.loop();

  // ส่งข้อมูลไป server ทุก PUBLISH_INTERVAL มิลลิวินาที
  unsigned long now = millis();
  if (now - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = now;

    // สร้างข้อความที่จะส่ง (เปลี่ยนเป็นข้อมูลจริงได้ เช่น sensor value)
    String payload = "{\"device\":\"esp8266\",\"uptime\":" + String(now / 1000) + "}";

    if (client.publish(TOPIC_PUB, payload.c_str())) {
      Serial.println("Published: " + payload);
    } else {
      Serial.println("Publish failed");
    }
  }
}
