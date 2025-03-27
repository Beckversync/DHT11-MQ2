#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif // ESP32
#endif // ESP8266

#include <Arduino_MQTT_Client.h>
#include <OTA_Firmware_Update.h>
#include <ThingsBoard.h>

#ifdef ESP8266
#include <Arduino_ESP8266_Updater.h>
#else
#ifdef ESP32
#include <Espressif_Updater.h>
#endif // ESP32
#endif // ESP8266

// Cấu hình sử dụng kết nối có mã hóa hay không (true/false)
#define ENCRYPTED false

// Thông tin firmware hiện tại của thiết bị: tiêu đề và phiên bản
constexpr char CURRENT_FIRMWARE_TITLE[] = "Firmware";
constexpr char CURRENT_FIRMWARE_VERSION[] = "1.2";

// Số lần thử tối đa khi tải xuống mỗi gói firmware qua MQTT
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;

// Kích thước mỗi gói firmware tải xuống qua MQTT
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;

constexpr char WIFI_SSID[] = "Beckhcmut";
constexpr char WIFI_PASSWORD[] = "12345678";

// Token truy cập ThingsBoard, xem hướng dẫn tại https://thingsboard.io/docs/getting-started-guides/helloworld/
constexpr char TOKEN[] = "f3e9aeqrc8j9dxtnsuij";

// Địa chỉ máy chủ ThingsBoard
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";

// Cổng MQTT sử dụng: 1883 cho kết nối không mã hóa, 8883 cho kết nối mã hóa
#if ENCRYPTED
constexpr uint16_t THINGSBOARD_PORT = 8883U;
#else
constexpr uint16_t THINGSBOARD_PORT = 1883U;
#endif

// Kích thước tối đa của gói tin gửi và nhận qua MQTT
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 512U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 512U;

// Baud rate cho cổng Serial debug
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

#if ENCRYPTED
// Chứng chỉ gốc để thiết lập kết nối SSL, thay đổi theo máy chủ của bạn
constexpr char ROOT_CERT[] = R"(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)";
#endif

// Khởi tạo client WiFi
#if ENCRYPTED
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif

// Khởi tạo client MQTT
Arduino_MQTT_Client mqttClient(espClient);

// Khởi tạo API cho OTA update
OTA_Firmware_Update<> ota;
const std::array<IAPI_Implementation*, 1U> apis = { &ota };

// Khởi tạo ThingsBoard với kích thước buffer đã định nghĩa
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

// Khởi tạo updater để flash firmware mới
#ifdef ESP8266
Arduino_ESP8266_Updater updater;
#else
#ifdef ESP32
Espressif_Updater<> updater;
#endif // ESP32
#endif // ESP8266

// Biến trạng thái cập nhật
bool currentFWSent = false;
bool updateRequestSent = false;

/// @brief Khởi tạo kết nối WiFi và đợi đến khi kết nối thành công
void InitWiFi() {
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to AP");
#if ENCRYPTED
  espClient.setCACert(ROOT_CERT);
#endif
}

/// @brief Kiểm tra lại kết nối WiFi, nếu mất kết nối sẽ gọi InitWiFi()
bool reconnect() {
  if (WiFi.status() == WL_CONNECTED)
    return true;
  InitWiFi();
  return true;
}

/// @brief Callback bắt đầu cập nhật OTA
void update_starting_callback() {
  Serial.println("Update starting callback called.");
}

/// @brief Callback khi hoàn thành quá trình OTA update (thành công hay thất bại)
void finished_callback(const bool & success) {
  if (success) {
    Serial.println("Firmware update completed successfully. Rebooting...");
#ifdef ESP8266
    ESP.restart();
#else
#ifdef ESP32
    esp_restart();
#endif // ESP32
#endif // ESP8266
  } else {
    Serial.println("Firmware update failed.");
  }
}

/// @brief Callback theo dõi tiến trình cập nhật (in phần trăm tiến độ và số chunk đã nhận)
void progress_callback(const size_t & current, const size_t & total) {
  float progress = (static_cast<float>(current) * 100) / total;
  Serial.printf("Update Progress: %.2f%% (%d/%d)\n", progress, current, total);
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(1000);
  InitWiFi();
}

void loop() {
  delay(1000);

  if (!reconnect()) {
    return;
  }

  if (!tb.connected()) {
    Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Failed to connect to ThingsBoard.");
      return;
    }
  }

  if (!currentFWSent) {
    currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
  }

  if (!updateRequestSent) {
    Serial.println("Firmware Update...");
    const OTA_Update_Callback callback(
      CURRENT_FIRMWARE_TITLE,
      CURRENT_FIRMWARE_VERSION,
      &updater,
      &finished_callback,
      &progress_callback,
      &update_starting_callback,
      FIRMWARE_FAILURE_RETRIES,
      FIRMWARE_PACKET_SIZE
    );
    updateRequestSent = ota.Start_Firmware_Update(callback);
  }

  tb.loop();
}


