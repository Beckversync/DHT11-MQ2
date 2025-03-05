#include <Arduino.h>
#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
#include <ArduinoOTA.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <DHT.h>

#define LED_PIN 48
// #define SDA_PIN GPIO_NUM_16
// #define SCL_PIN GPIO_NUM_17

#define DHT_PIN  16  
#define DHT_TYPE DHT11 
DHT dht(DHT_PIN, DHT_TYPE); 

#define MQ2_PIN 34


constexpr char WIFI_SSID[] = "Beckhcmut";
constexpr char WIFI_PASSWORD[] = "12345678";

constexpr char TOKEN[] = "f3e9aeqrc8j9dxtnsuij";

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";

volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

constexpr int16_t telemetrySendInterval = 10000; // ms


WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);
// DHT20 dht20;


RPC_Response setLedSwitchState(const RPC_Data &data) {
  Serial.println("Received Switch state");
  bool newState = data;
  Serial.print("Switch state change: ");
  Serial.println(newState);
  digitalWrite(LED_PIN, newState);
  attributesChanged = true;
  return RPC_Response("setLedSwitchValue", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {{
  RPC_Callback{ "setLedSwitchValue", setLedSwitchState }
}};

void processSharedAttributes(const Shared_Attribute_Data &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0) {
      const uint16_t new_interval = it->value().as<uint16_t>();
      if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX) {
        blinkingInterval = new_interval;
        Serial.print("Blinking interval is set to: ");
        Serial.println(new_interval);
      }
    } else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0) {
      ledState = it->value().as<bool>();
      digitalWrite(LED_PIN, ledState);
      Serial.print("LED state is set to: ");
      Serial.println(ledState);
    }
  }
  attributesChanged = true;
}

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, 
  std::begin({LED_STATE_ATTR, BLINKING_INTERVAL_ATTR}), std::end({LED_STATE_ATTR, BLINKING_INTERVAL_ATTR}));
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, 
  std::begin({LED_STATE_ATTR, BLINKING_INTERVAL_ATTR}), std::end({LED_STATE_ATTR, BLINKING_INTERVAL_ATTR}));


void InitWiFi() {
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Chờ kết nối ()
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("\nConnected to AP");
}




void WifiTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, attempting reconnect...");
      InitWiFi();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


void CoreIOTTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (!tb.connected()) {
      Serial.print("Connecting to: ");
      Serial.print(THINGSBOARD_SERVER);
      Serial.print(" with token ");
      Serial.println(TOKEN);
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
        Serial.println("Failed to connect to ThingsBoard");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        continue;
      }
      tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

      Serial.println("Subscribing for RPC...");
      if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
        Serial.println("Failed to subscribe for RPC");
      }
      if (!tb.Shared_Attributes_Subscribe(attributes_callback)) {
        Serial.println("Failed to subscribe for shared attribute updates");
      }
      if (!tb.Shared_Attributes_Request(attribute_shared_request_callback)) {
        Serial.println("Failed to request shared attributes");
      }
      Serial.println("Subscription done");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TelemetryTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    vTaskDelay(telemetrySendInterval / portTICK_PERIOD_MS);
    
    dht.read();
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int mq2Value = analogRead(MQ2_PIN); 
    if (isnan(temperature) || isnan(humidity)|| isnan(mq2Value)) {
      Serial.println("Failed to read from DHT20 sensor!");
    } else {
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
      Serial.print("MQ2 sensor value: ");
      Serial.println(mq2Value);

      tb.sendTelemetryData("temperature", temperature);
      tb.sendTelemetryData("humidity", humidity);
      tb.sendTelemetryData("sensor",mq2Value);
    }

    tb.sendAttributeData("rssi", WiFi.RSSI());
    tb.sendAttributeData("channel", WiFi.channel());
    tb.sendAttributeData("bssid", WiFi.BSSIDstr().c_str());
    tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
    tb.sendAttributeData("ssid", WiFi.SSID().c_str());
  }
}


void AttributesTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (attributesChanged) {
      attributesChanged = false;
      int currentLEDState = digitalRead(LED_PIN);
      Serial.print("Sending LED state attribute: ");
      Serial.println(currentLEDState);
      tb.sendAttributeData(LED_STATE_ATTR, currentLEDState);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}




void TBLoopTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    tb.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ---------------------- setup() ----------------------
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  pinMode(LED_PIN, OUTPUT);
  delay(1000);
  

  InitWiFi();
//   Wire.begin(SDA_PIN, SCL_PIN);
  dht.begin();


  xTaskCreate(WifiTask, "WifiTask", 4096, NULL, 1, NULL);
  xTaskCreate(CoreIOTTask, "CoreIOTTask", 4096, NULL, 1, NULL);
  xTaskCreate(TelemetryTask, "TelemetryTask", 4096, NULL, 1, NULL);
  xTaskCreate(AttributesTask, "AttributesTask", 4096, NULL, 1, NULL);
  xTaskCreate(TBLoopTask, "TBLoopTask", 4096, NULL, 1, NULL);
//   xTaskCreate(MQ2Task, "MQ2Task", 2048, NULL, 1, NULL);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
