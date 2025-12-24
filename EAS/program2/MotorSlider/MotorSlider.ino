#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "TAHU";
const char* password = "emokmales123";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* topic_control = "iot/motor/control";
const char* topic_speed = "iot/motor/speed";

int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 12;

const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 0;
bool motorState = false;  // Status motor (ON/OFF)

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(" | Message: ");
  Serial.println(message);
  
  // Kontrol ON/OFF motor
  if (String(topic) == topic_control) {
    if (message == "1") {
      // MOTOR ON dengan kecepatan terakhir (dutyCycle)
      digitalWrite(motor1Pin1, HIGH);
      digitalWrite(motor1Pin2, LOW);
      ledcWrite(pwmChannel, dutyCycle);
      motorState = true;
      Serial.println("🚀 Motor ON");
      Serial.print("Kecepatan: ");
      Serial.print(dutyCycle);
      Serial.print(" (");
      Serial.print(map(dutyCycle, 0, 255, 0, 100));
      Serial.println("%)");
    } 
    else if (message == "0") {
      // MOTOR OFF
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, LOW);
      ledcWrite(pwmChannel, 0);
      motorState = false;
      Serial.println("🛑 Motor OFF");
    }
  }
  
  // Kontrol kecepatan
  if (String(topic) == topic_speed) {
    int speedVal = message.toInt();
    
    // Cek apakah nilai valid (0-255)
    if (speedVal >= 0 && speedVal <= 255) {
      dutyCycle = speedVal;
      Serial.print("Kecepatan diatur ke: ");
      Serial.print(dutyCycle);
      Serial.print(" (");
      Serial.print(map(dutyCycle, 0, 255, 0, 100));
      Serial.println("%)");
      
      // Jika motor sedang ON, langsung terapkan kecepatan
      if (motorState) {
        ledcWrite(pwmChannel, dutyCycle);
        Serial.println("Motor sedang ON - kecepatan diterapkan");
      } else {
        Serial.println("Motor OFF - kecepatan disimpan untuk next ON");
      }
    } else {
      Serial.print("⚠️ Nilai kecepatan invalid: ");
      Serial.println(speedVal);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32MotorClient")) {
      Serial.println("connected");
      client.subscribe(topic_control);
      client.subscribe(topic_speed);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(enable1Pin, pwmChannel);
  
  // Pastikan motor mati saat startup
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  ledcWrite(pwmChannel, 0);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}