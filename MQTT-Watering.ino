#include     <ESP8266WiFi.h>
#include     <PubSubClient.h>
#include     <ESP8266HTTPClient.h>
#include     <ArduinoJson.h>

//Set up imports
WiFiClient   espClient;
PubSubClient mqtt(espClient);
HTTPClient   http;

//Input channels
#define      VALVE2_RELAY     D1
#define      VALVE1_RELAY    D2

//WifiSettings
const char*  SSID           = "SSID";
const char*  PASSWORD       = "PWD";
const char*  MQTT_SERVER    = "1.2.3.4";
const int    BAUD_RATE      = 115200;

//Global variables
bool          openValve1             = false;
bool          openValve2             = false;

unsigned long elapsedMillis1         = 0;
unsigned long elapsedMillis2         = 0;
unsigned long timer1                 = 10*60*1000;
unsigned long timer2                 = 10*60*1000;

void wifiSetup() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200); Serial.print(".");
  }
  Serial.println("Connected: local ip address is:");
  Serial.println(WiFi.localIP());
}

void callback(
  char* topic,
  byte* payload,
  unsigned int length
) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg = msg+(char)payload[i];
  }

  StaticJsonDocument<200> doc;
  deserializeJson(doc, msg);
  int whichValve = doc["valve"];

  if (strcmp(topic, "watering/valve") == 0 && whichValve == 1) {
    elapsedMillis1 = 0;
    timer1         = doc["time"];
    timer1         = timer1*1000;
    openValve1     = true;
  }

  if (strcmp(topic, "watering/valve") == 0 && whichValve == 2) {
    elapsedMillis2 = 0;
    timer2         = doc["time"];
    timer2         = timer2*1000;
    openValve2     = true;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      mqtt.subscribe("watering/valve");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  //setup Relays
  pinMode(VALVE2_RELAY, OUTPUT);
  pinMode(VALVE1_RELAY, OUTPUT);
  //Relay initial state.
  digitalWrite(VALVE2_RELAY, HIGH);
  digitalWrite(VALVE1_RELAY, HIGH);
  //Communication
  Serial.begin(BAUD_RATE);
  wifiSetup();
  mqtt.setServer(MQTT_SERVER, 1883);
  mqtt.setCallback(callback);
}

bool valve1Open(unsigned long elapsedMillis1) {
  unsigned long t = millis() - elapsedMillis1;
  if (t < timer1) {
    digitalWrite(VALVE1_RELAY, LOW);
    Serial.println(t);
  } else  {
    digitalWrite(VALVE1_RELAY, HIGH);
    elapsedMillis1 = 0;
    openValve1     = false;

    return true;
  }

  return false;
}

bool valve2Open(unsigned long elapsedMillis2) {
  unsigned long t = millis() - elapsedMillis2;
  if (t < timer2) {
    Serial.println(t);
    digitalWrite(VALVE2_RELAY, LOW);
  } else  {
    digitalWrite(VALVE2_RELAY, HIGH);
    elapsedMillis2 = 0;
    openValve2     = false;

    return true;
  }

  return false;
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  if (openValve1 == true) {
    if (elapsedMillis1 == 0) {
      elapsedMillis1 = millis();
    }
    if (valve1Open(elapsedMillis1)) {

    }
  }

  if (openValve2 == true) {
    if (elapsedMillis2 == 0) {
      elapsedMillis2 = millis();
    }
    if (valve2Open(elapsedMillis2)) {

    }
  }

  mqtt.loop();
}
