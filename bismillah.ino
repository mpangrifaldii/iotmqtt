#include <ESP8266MQTTClient.h>
#include <ESP8266WiFi.h>
#include <dht11.h>
#include <pt.h>
#include <ArduinoJson.h>

#define triggerPin  D5
#define echoPin     D6
#define DHT11PIN D3
#define ledPin D4
dht11 DHT11;

MQTTClient mqtt;

static struct pt pt1, pt2, pt3; // each protothread needs one of these

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(triggerPin, OUTPUT);
 
  WiFi.mode(WIFI_STA);
  WiFi.begin("Redmi Note 9 Pro", "Lagirusak");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  PT_INIT(&pt1);  // initialise the two
  PT_INIT(&pt2);  // initialise the two
  PT_INIT(&pt3);  // initialise the two

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  mqtt.onDisconnect([]() {
    Serial.printf("MQTT: Disconnected\r\n");
  });

  //topic, data, data is continuing
  mqtt.onData([](String topic, String data, bool cont) {
    Serial.printf("Data received, topic: %s, data: %s\r\n", topic.c_str(), data.c_str());
    led(topic.c_str(), data.c_str());
  });

  mqtt.onSubscribe([](int sub_id) {
    Serial.printf("Subscribe topic id: %d ok\r\n", sub_id);
  });
  mqtt.onConnect([]() {
    Serial.printf("MQTT: Connected\r\n");
    mqtt.subscribe("uas/iot/+", 1);
  });

//  mqtt.begin("ws://broker.mqttdashboard.com:8000/mqtt");
//  mqtt.begin("ws://broker.emqx.io:8083/mqtt");
  mqtt.begin("ws://test.mosquitto.org:8080", {.lwtTopic = "hello", .lwtMsg = "offline", .lwtQos = 0, .lwtRetain = 0});
}

void led(String topic, String data){
  if(topic == "ledcontrol"){
    Serial.println("topik");
    if(data == "ON"){
      digitalWrite(ledPin, HIGH);
      Serial.println("nyala");
    }else if(data == "OFF"){
      digitalWrite(ledPin, LOW);
      Serial.println("mati");
    }
  }
}

static int protothread2(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  StaticJsonDocument<300> doc;
//  JsonObject JSONencoder = doc.to<JsonObject>();
  PT_BEGIN(pt);
  while(1) { // never stop
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp
    int chk = DHT11.read(DHT11PIN);
    float humi = (float)DHT11.humidity;
    float temp = (float)DHT11.temperature;
    Serial.print("Humidity (%): ");
    Serial.println(humi, 2);
    Serial.print("Temperature (C): ");
    Serial.println(temp, 2);
    mqtt.publish("value1_iot", String(humi), 1, 1);
    mqtt.publish("value2_iot", String(temp), 1, 1);
  }
  PT_END(pt);
}

static int protothread3(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;

  PT_BEGIN(pt);
  while(1) { // never stop
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp
    float duration, jarak;
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2); 
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10); 
    digitalWrite(triggerPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    jarak = (duration/2) / 29.1;
    Serial.print("Jarak : ");
    Serial.print(jarak, 2);
    Serial.println(" cm");
    mqtt.publish("value3_iot", String(jarak), 1, 1);
  }
  PT_END(pt);
}

void loop() {
  mqtt.handle();
//  protothread1(&pt1, 10100); // schedule the two protothreads
  protothread2(&pt2, 10000); // schedule the two protothreads
  protothread3(&pt3, 5000); // schedule the two protothreads
}
