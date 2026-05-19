#define BLYNK_TEMPLATE_NAME "ESP8266V1"
#define BLYNK_AUTH_TOKEN "EM3gAZ4dL23ZmVedD4AC3KQfZhxYM6ec"
#define BLYNK_TEMPLATE_ID "TMPL6Ky4pfa-b"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#define BLYNK_PRINT Serial

#define LED_PIN 2
#define BUTTON_PIN D2
#define LDR_PIN A0
#define RED_PIN 15
#define GREEN_PIN 12
#define BLUE_PIN 13
#define BAUD_RATE 115200
#define DHT_PIN D4
#define DHT_TYPE DHT11

char ssid[] = "vivo V21 5G";
char password[] = "52835283";

BlynkTimer timer;
bool ledState = false;

// BlynkState state;
// bool buttonState=false;
//

DHT dht(DHT_PIN, DHT_TYPE);

void timerEvent() {
  int ldr = analogRead(LDR_PIN);
  Blynk.virtualWrite(V1, ldr);
  Serial.print("LDR Value: ");
  Serial.println(ldr);

  int buttonState = digitalRead(BUTTON_PIN);
if(buttonState == LOW){
  Blynk.virtualWrite(V5, 1);
}
else{
  Blynk.virtualWrite(V5, 0);
}

float temp = dht.readTemperature();
float humidity = dht.readHumidity();
Blynk.virtualWrite(V8, temp);
float tempF=temp*1.8+32;

Blynk.virtualWrite(V9, tempF);
Blynk.virtualWrite(V11, humidity);
  

}

BLYNK_WRITE(V0){
  int value = param.asInt();
  if(value > 0 && !ledState){
    digitalWrite(LED_PIN, LOW);
    ledState = !ledState;
  }
  else if (value == 0 && ledState){
    digitalWrite(LED_PIN, HIGH);
    ledState = !ledState;
  }
}

BLYNK_WRITE(V2) {
  int redValue = param.asInt();
  analogWrite(RED_PIN, redValue); 
}

BLYNK_WRITE(V3) {
  int greenValue = param.asInt();
  analogWrite(GREEN_PIN, greenValue);
}

BLYNK_WRITE(V4) {
  int blueValue = param.asInt();
  analogWrite(BLUE_PIN, blueValue);
}

void setup() {
dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, HIGH); // ปิดไฟ LED (Active Low)
  
  Serial.begin(BAUD_RATE);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(1000L, timerEvent);
}

void loop() {
  Blynk.run();
  timer.run();
}
