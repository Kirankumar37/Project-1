#include "esp_camera.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "Airtel_SUNRISE MEN'S PG";
const char* password = "sunrise@2024";

// Define LED GPIOs
#define LEFT_RED 12   // Left side RED light
#define LEFT_GREEN 13 // Left side GREEN light
#define RIGHT_RED 14  // Right side RED light
#define RIGHT_GREEN 15 // Right side GREEN light

// Create AsyncWebServer
AsyncWebServer server(80);

void startCameraServer();

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    // Configure LED pins
    pinMode(LEFT_RED, OUTPUT);
    pinMode(LEFT_GREEN, OUTPUT);
    pinMode(RIGHT_RED, OUTPUT);
    pinMode(RIGHT_GREEN, OUTPUT);

    // Initialize all LEDs to GREEN (default state)
    digitalWrite(LEFT_RED, LOW);
    digitalWrite(LEFT_GREEN, HIGH);
    digitalWrite(RIGHT_RED, LOW);
    digitalWrite(RIGHT_GREEN, HIGH);

    // Camera setup
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("Camera Ready! Access it at: http://");
    Serial.println(WiFi.localIP());

    // Route to serve camera image
    server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            request->send(500, "text/plain", "Camera capture failed");
            return;
        }
        request->send_P(200, "image/jpeg", fb->buf, fb->len);
        esp_camera_fb_return(fb);
    });

    // Control traffic lights based on Python script request
    server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(LEFT_RED, LOW);
        digitalWrite(LEFT_GREEN, HIGH);
        digitalWrite(RIGHT_RED, HIGH);
        digitalWrite(RIGHT_GREEN, LOW);
        Serial.println("Vehicle detected on left: RED on right, GREEN on left");
        request->send(200, "text/plain", "Left detected");
    });

    server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(LEFT_RED, HIGH);
        digitalWrite(LEFT_GREEN, LOW);
        digitalWrite(RIGHT_RED, LOW);
        digitalWrite(RIGHT_GREEN, HIGH);
        Serial.println("Vehicle detected on right: RED on left, GREEN on right");
        request->send(200, "text/plain", "Right detected");
    });

    server.on("/both", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(LEFT_RED, HIGH);
        digitalWrite(LEFT_GREEN, LOW);
        digitalWrite(RIGHT_RED, HIGH);
        digitalWrite(RIGHT_GREEN, LOW);
        Serial.println("Vehicles detected on both sides: RED lights ON both sides");
        request->send(200, "text/plain", "Both detected");
    });

    server.on("/none", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(LEFT_RED, LOW);
        digitalWrite(LEFT_GREEN, HIGH);
        digitalWrite(RIGHT_RED, LOW);
        digitalWrite(RIGHT_GREEN, HIGH);
        Serial.println("No vehicle detected: GREEN lights ON both sides");
        request->send(200, "text/plain", "No detection");
    });

    // Start the server
    server.begin();
}

void loop() {
    // No need for loop actions, handled by the webserver
}
