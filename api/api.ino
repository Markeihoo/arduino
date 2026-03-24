#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define BAUD_RATE 115200
#define SSID "vivo V21 5G"
#define PASSWORD "52835283"
#define MQTT_PORT 1883

#define RED_PIN   15
#define GREEN_PIN 12
#define BLUE_PIN  13
#define PUSH_BUTTON D2
#define LDR A0

const char * MQTT_SERVER = "mqtt.netpie.io";
const char * CLIENT_ID ="8f558985-2fe9-40f4-9dbc-f0dd438ad2f4";
const char * TOKEN = "W8yjwydUfPgSFcTpub5A927AXfjQ1yhn";
const char * SECRET = "8UYbowaWu9mFJe9YVbw27zmstiXNcyAy";

const char * TOPIC_SHADOW= "@shadow/data/update";
const char * TOPIC_LED= "@msg/led";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
const char * MY_CLIENT_ID = "natthawut";

bool buttonState = false;
bool lastButtonReading = HIGH;
unsigned long lastCooldownTime = 0;
const unsigned long COOLDOWN = 200;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 500;
String ledStatus = "";
String currentStatus = "";

void callback(char * topic, byte * payload, unsigned int length) {
  char * msg = (char *) malloc (length + 1);
  memcpy(msg, payload,length);
  msg[length] = '\0';
  String str = String(msg);
  Serial.printf("Message received: %s\n",str.c_str());
  free(msg);

  if (String(topic) == TOPIC_LED) {
    if (str == "on") {
      digitalWrite(RED_PIN, HIGH);
      ledStatus = "on";
      Serial.printf("LED is %s", ledStatus);
    } else if (str == "off") {
      digitalWrite(RED_PIN, LOW);
      ledStatus = "off";
      Serial.printf("LED is %s", ledStatus);
    }
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  digitalWrite(RED_PIN, LOW);

  WiFi.begin(SSID,PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print('.');
  }

  Serial.println("\n\nWIFI connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_SERVER,MQTT_PORT);
  client.setCallback(callback);
}

void reconnect(){
  while (!client.connected()){
    Serial.println("Connecting to Broker....");
    if(client.connect(CLIENT_ID, TOKEN, SECRET)){
      Serial.println("Connected to MQTT Broker!");
      client.subscribe(TOPIC_SHADOW);
      client.subscribe(TOPIC_LED);
    }else {
      Serial.print("Failed to connected: ");
      Serial.println(client.state());
      delay(500);
    }
  }
}

void loop() {
  if(!client.connected()){
    reconnect();
  }

  // cooldown + button
  if (millis() - lastCooldownTime >= COOLDOWN) {
    bool reading = digitalRead(PUSH_BUTTON);
    if (reading != lastButtonReading) {
      lastCooldownTime = millis();
      lastButtonReading = reading;
      buttonState = (reading == LOW);

      int ldrValue = analogRead(LDR);
      String data = String("{\"data\":{") +
                    String("\"status\":\"") + currentStatus + String("\",") +
                    String("\"ldr\":") + String(ldrValue) + String(",") +
                    String("\"button\":") + (buttonState ? "true" : "false") +
                    String("}}");
      client.publish(TOPIC_SHADOW, data.c_str());
    }
  }

  if(Serial.available() > 0){
    String str = Serial.readStringUntil('\n');
    str.trim();
    currentStatus = str;

    int ldrValue = analogRead(LDR);
    String data = String("{\"data\":{") +
                  String("\"status\":\"") + currentStatus + String("\",") +
                  String("\"ldr\":") + String(ldrValue) + String(",") +
                  String("\"button\":") + (buttonState ? "true" : "false") +
                  String("}}");
    client.publish(TOPIC_SHADOW, data.c_str());
  }

  if (millis() - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = millis();
    int ldrValue = analogRead(LDR);

    String data = String("{\"data\":{") +
                  String("\"status\":\"") + currentStatus + String("\",") +
                  String("\"ldr\":") + String(ldrValue) + String(",") +
                  String("\"button\":") + (buttonState ? "true" : "false") +
                  String("}}");
    client.publish(TOPIC_SHADOW, data.c_str());
  }

  client.loop();
}
