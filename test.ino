#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define BAUD_RATE 115200
#define SSID ".1"
#define PASSWORD "17061706"
#define MQTT_PORT 1883

const char * NQTT_SERVER = "mqtt.netpie.io";
const char * CLIENT_ID = "";
const char * TOKEN = "";
const char * SECRET = "";

const char * TOPIC = "/dpu/cite/ce379/saravut/test";

WiFiClient WiFiClient;
PubSubClient client(WiFiClient);
const char * MY_CLIENT_ID="cite:kanun";

void setup() {
Serial.begin(BAUD_RATE);
WiFi.begin(SSID, PASSWORD);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("\n\n WiFi connected");
Serial.println("WiFi address: ");
Serial.println(WiFi.localIP());
client.setServer(NQTT_SERVER, MQTT_PORT);
client.setCallback(callback);

while(!client.connected()){
Serial.print("Connecting to MQTT broker.....");
if(client.connect(MY_CLIENT_ID)){
Serial.println("Connected to MQTT Broker");
}else{
Serial.print("failed with state ");
Serial.print(client.state());
delay(500);
}
}
}

void callback(char* topic,byte * payload , unsigned int length){
  
}


void loop(){

}
