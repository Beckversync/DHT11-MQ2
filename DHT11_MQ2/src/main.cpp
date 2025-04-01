#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #ifdef ESP32
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
  #endif
#endif

#include <Arduino_MQTT_Client.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ----- ĐỊNH NGHĨA CHÂN & CẢM BIẾN -----
#define DHT_PIN         16    
#define DHT_TYPE        DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ----- CẤU HÌNH KẾT NỐI -----
#define ENCRYPTED       false

constexpr char WIFI_SSID[] = "Beckhcmut";
constexpr char WIFI_PASSWORD[] = "12345678";
constexpr char TOKEN[] = "f3e9aeqrc8j9dxtnsuij";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint16_t MAX_MESSAGE_SEND_SIZE    = 256U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;
constexpr uint32_t SERIAL_DEBUG_BAUD        = 115200U;
constexpr size_t MAX_ATTRIBUTES             = 1U;

constexpr const char DHT_SCHEDULE_KEY[] = "Scheduler";

// ----- NTP Client -----
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

// ----- WiFi & ThingsBoard -----
#if ENCRYPTED
  WiFiClientSecure espClient;
#else
  WiFiClient espClient;
#endif

Arduino_MQTT_Client mqttClient(espClient);
Shared_Attribute_Update<1U, MAX_ATTRIBUTES> shared_update;
const std::array<IAPI_Implementation*, 1U> apis = { &shared_update };
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

bool subscribed = false;
bool isDhtEnabled = false;

// ----- Lịch -----
struct TimeSlot {
  String start;
  String end;
};
std::vector<TimeSlot> dhtSchedule;

bool isInTimeRange(const String &current, const String &start, const String &end) {
  return current >= start && current < end;
}

bool shouldDhtRun(const String &currentTime) {
  for (auto &slot : dhtSchedule) {
    if (isInTimeRange(currentTime, slot.start, slot.end)) {
      return true;
    }
  }
  return false;
}

// ----- Hàm khởi tạo WiFi -----
void InitWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
#if ENCRYPTED

#endif
}

bool reconnect() {
  if (WiFi.status() == WL_CONNECTED) return true;
  InitWiFi();
  return true;
}




void DHTTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    Serial.println("DHT Task active: " + String(isDhtEnabled ? "YES" : "NO"));
    if (isDhtEnabled) {
      float temperature = dht.readTemperature();
      float humidity    = dht.readHumidity();

      if (isnan(temperature) || isnan(humidity)) {
        Serial.println(" Failed to read from DHT11 sensor!");
      } else {
        Serial.printf(" Temp: %.2f °C,  Humidity: %.2f %%\n", temperature, humidity);
        if (tb.connected()) {
          tb.sendTelemetryData("temperature", temperature);
          tb.sendTelemetryData("humidity", humidity);
        }
      }
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}


// ----- TASK: DhtScheduleTask  -----
void DhtScheduleTask(void *pvParameters) {
  (void) pvParameters;
  timeClient.begin();
  for (;;) {
    timeClient.update();
    String currentTime = timeClient.getFormattedTime().substring(0, 5);
    Serial.println("Current time: " + currentTime);
    isDhtEnabled = shouldDhtRun(currentTime);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void CoreIOTTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (!tb.connected()) {
      Serial.printf("Connecting to ThingsBoard at %s with token %s\n", THINGSBOARD_SERVER, TOKEN);
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
        Serial.println("Failed to connect to ThingsBoard");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        continue;
      }
      tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    }
    if (!subscribed) {
      Serial.println("Subscribing to dht schedule attribute...");
      constexpr std::array<const char*, MAX_ATTRIBUTES> keys = { DHT_SCHEDULE_KEY };
      const Shared_Attribute_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeUpdate, keys);
      if (!shared_update.Shared_Attributes_Subscribe(callback)) {
        Serial.println("Failed to subscribe to dht schedule");
      } else {
        subscribed = true;
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void WiFiTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, reconnecting...");
      InitWiFi();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TBLoopTask(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    tb.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  dht.begin();
  InitWiFi();
  timeClient.begin();

  xTaskCreate(WiFiTask,        "WiFiTask",        4096, NULL, 1, NULL);
  xTaskCreate(CoreIOTTask,     "CoreIOTTask",     4096, NULL, 1, NULL);
  xTaskCreate(DHTTask,         "DHTTask",         4096, NULL, 1, NULL);
  xTaskCreate(DhtScheduleTask, "ScheduleTask",    4096, NULL, 1, NULL);
  xTaskCreate(TBLoopTask,      "TBLoopTask",      4096, NULL, 1, NULL);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}