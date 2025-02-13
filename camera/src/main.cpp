/**********************************************************************
  Filename    : Camera Web Server
  Description : The camera images captured by the ESP32S3 are displayed on the web page.
  Auther      : www.freenove.com
  Modification: 2024/07/01
**********************************************************************/
#include "esp_camera.h"
#include <WiFi.h>
#include <esp_log.h>
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
#include "camera_pins.h"

extern "C" {
  #include "mqtt_client.h"
}

// Replace these with your actual PEM certificates
const char *mqtt_ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDiTCCAnGgAwIBAgIUXy/lbGPAIz46ex0VZGMVj3mYBB4wDQYJKoZIhvcNAQEL\
BQAwVDELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxFzAVBgNVBAoM\
DkZha2UgQXV0aG9yaXR5MRcwFQYDVQQDDA4xOTIuMTY4LjUwLjE5MjAeFw0yNTAy\
MTIxNjQ2MTZaFw0zMDAyMTIxNjQ2MTZaMFQxCzAJBgNVBAYTAkFVMRMwEQYDVQQI\
DApTb21lLVN0YXRlMRcwFQYDVQQKDA5GYWtlIEF1dGhvcml0eTEXMBUGA1UEAwwO\
MTkyLjE2OC41MC4xOTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC7\
o+SG35L2tIZRe+rgo5I/JxUfOBj4/H59OrY8k2e/3UKY1+k4xA6Frv/X5K7El4xm\
C+eAVsFFv8k56FZ1XbRyVJDCfAhEQ5dzcqw9YmNuB/IAvdFpiJvxKZTlT+mQcuxL\
SmvmWj+ixmpgbXxzQ2GGbZ6CoPc0YpeoxCft6cYOxKBBKvwVpBxkfb0tuoOg3jxZ\
ZQRgzx01pqrMYDRJjVHzTsnY9ocU/6VT7sfv+UVIaf/IaC5l+PhoWmf1cibwWKaI\
y3w1j5y+AK29MU1SCgLtRXigoyckAI06PNgJv9LhSt3ObncYrwWZ7KVT9UVqljP+\
8fQQftcKfRUCWjs+qfbXAgMBAAGjUzBRMB0GA1UdDgQWBBSJmSSRZKHfiYBupJ5Z\
tWEjAkFffTAfBgNVHSMEGDAWgBSJmSSRZKHfiYBupJ5ZtWEjAkFffTAPBgNVHRMB\
Af8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBEDISA9RbrHRp5gdsPGECc/rhO\
4h94M+6625TG0X2NZwUinl8YRoWF5kBuQP9ejTRgSmIij4CDrL3lUvc7TtiiJMDN\
CihuP1RxDLwpoFR808guVo1t08rQa8c0LDJWkeTVKnmtRyfbs0UlLbmPKH3SbQKO\
s8+7hZC9UDGSspUfN67OMrYg4JWj1rQdklCRKyZkAizT/NWntuX7UfLBN9hKFd4l\
pIE4qLYj+tQuqhb6LVy/K3U8FmlUOhTaMiw2RwC68iz6um29WSEztxh/Po1Tu/RU\
F6kyd5bqwO03ksJUiHl2HTHka4+nmX3K6uYmRr18JfuXjvW7ijSEueHPxhJw\n" \
"-----END CERTIFICATE-----\n";

// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid     = "EG106";
const char* password = "pula1234";
const char* mqtt_broker   = "192.168.0.100";
const int   mqtt_port     = 8883;
const char* mqtt_topic   = "esp32/camera";

esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    switch(event_id) {
      case MQTT_EVENT_CONNECTED:
        Serial.println("MQTT_EVENT_CONNECTED");
        break;
      case MQTT_EVENT_DISCONNECTED:
        Serial.println("MQTT_EVENT_DISCONNECTED");
        break;
      case MQTT_EVENT_PUBLISHED:
        Serial.printf("MQTT_EVENT_PUBLISHED, msg_id=%d\n", event->msg_id);
        break;
      case MQTT_EVENT_ERROR:
        Serial.println("MQTT_EVENT_ERROR");
        break;
      default:
        break;
    }
    //return ESP_OK;
}

void init_mqtt() {
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.host = mqtt_broker;
  mqtt_cfg.port = mqtt_port;
  mqtt_cfg.transport = MQTT_TRANSPORT_OVER_SSL;
  // Set mTLS credentials:
  mqtt_cfg.cert_pem = mqtt_ca_cert;
  //mqtt_cfg.client_cert_pem = mqtt_client_cert;
  //mqtt_cfg.client_key_pem = mqtt_client_key;
  
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(mqtt_client);
}

void setup_camera() {
  camera_config_t config = {};
  config.ledc_channel    = LEDC_CHANNEL_0;
  config.ledc_timer      = LEDC_TIMER_0;
  config.pin_d0          = Y2_GPIO_NUM;
  config.pin_d1          = Y3_GPIO_NUM;
  config.pin_d2          = Y4_GPIO_NUM;
  config.pin_d3          = Y5_GPIO_NUM;
  config.pin_d4          = Y6_GPIO_NUM;
  config.pin_d5          = Y7_GPIO_NUM;
  config.pin_d6          = Y8_GPIO_NUM;
  config.pin_d7          = Y9_GPIO_NUM;
  config.pin_xclk        = XCLK_GPIO_NUM;
  config.pin_pclk        = PCLK_GPIO_NUM;
  config.pin_vsync       = VSYNC_GPIO_NUM;
  config.pin_href        = HREF_GPIO_NUM;
  config.pin_sccb_sda    = SIOD_GPIO_NUM;
  config.pin_sccb_scl    = SIOC_GPIO_NUM;
  config.xclk_freq_hz    = 20000000;
  config.pixel_format    = PIXFORMAT_JPEG;
  
  // Use PSRAM if available
  if (psramFound()) {
    config.frame_size    = FRAMESIZE_UXGA;
    config.jpeg_quality  = 10; 
    config.fb_count      = 2;
  } else {
    config.frame_size    = FRAMESIZE_SVGA;
    config.jpeg_quality  = 12; 
    config.fb_count      = 1;
    config.fb_location   = CAMERA_FB_IN_DRAM;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println();
  Serial.println();
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize camera and MQTT
  setup_camera();
  init_mqtt();
}

void loop() {
  // Capture an image from the camera
  camera_fb_t *pic = esp_camera_fb_get();
  if (pic) {
    Serial.printf("Picture taken! Its size: %zu bytes\n", pic->len);
    // Publish JPEG image over MQTT using ESP-IDF MQTT client
    int msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_topic, (const char*)pic->buf, pic->len, 1, 0);
    Serial.printf("Published message, msg_id=%d\n", msg_id);
    esp_camera_fb_return(pic);
  } else {
    Serial.println("Camera capture failed");
  }
  delay(5000); // 10-second delay between captures
}