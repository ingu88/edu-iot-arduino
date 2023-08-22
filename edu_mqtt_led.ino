#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

const int pdid = 1; // 디바이스 아이디(시리얼넘버)
const String pdname = "IoT_Switch"; // 디바이스 이름

// 변경될 변수
String mqtt_server = "broker.hivemq.com";
String mqtt_topic = "dongwon/iot/1";
int led1_state = LOW;   // the current state of LED
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// 원격제어 영역 : mqtt 응답값 받기
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // 단순 출력
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  DynamicJsonDocument json(1024);
  // 수신한 JSON 데이터를 파싱해서 DynamicJsonDocument에 저장
  DeserializationError error = deserializeJson(json, payload, length);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  String sender = json["sender"];
  int on_off = json["on_off"];

  // on_off 적용
  if(on_off == 1){
    led1_state = LOW;
    digitalWrite(LED_BUILTIN, led1_state);
  }

  if(on_off == 0){
    led1_state = HIGH;
    digitalWrite(LED_BUILTIN, led1_state);
  }

  // 적용 후 보고
  if (sender == "system") {    
    DynamicJsonDocument res(256);
    res["sender"] = pdname;
    res["on_off"] = on_off;
    char buffer[256];
    size_t n = serializeJson(res, buffer);
    client.publish((char*)mqtt_topic.c_str(), buffer, n);
  }
}

// mqtt 재접속
void mqtt_reconnect() {
  while(!client.connected()) {
    Serial.print("MQTT connecting...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())){
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
  client.subscribe((char*)mqtt_topic.c_str());
}

void setup() {
  Serial.begin(9600);         // initialize serial
  pinMode(LED_BUILTIN, OUTPUT);  // set ESP32 pin to output mode

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //재부팅 후 와이파이 정보 지울때 주석 풀기
//  wifiManager.resetSettings();

  if (!wifiManager.autoConnect("dongwon_iot")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
  Serial.println("wifi connected ^^");
  Serial.print("local ip : ");
  Serial.println(WiFi.localIP());

  client.setServer((char*)mqtt_server.c_str(), 1883);
  client.setCallback(mqtt_callback);
  client.subscribe((char*)mqtt_topic.c_str());
}

void loop() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();
}
