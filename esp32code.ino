#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ------------------ WiFi Credentials ------------------
const char* ssid = "";
const char* password = "";

// ------------------ WebSocket ------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ------------------ MPU6050 ------------------
Adafruit_MPU6050 mpu;

// ------------------ DHT11 ------------------
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ------------------ Battery Measurement ------------------
#define BATTERY_PIN 34  // ADC pin for battery measurement

// ------------------ RTOS Tasks ------------------
TaskHandle_t sensorTaskHandle;
TaskHandle_t sendTaskHandle;

// ------------------ Mutex for data sharing ------------------
SemaphoreHandle_t dataMutex;

// ------------------ Shared Telemetry Structure ------------------
struct TelemetryData {
  float pitch, roll, yaw;
  float temperature;
  float humidity;
  float batteryVoltage;
} telemetry;

// ------------------ JSON Sender ------------------
void notifyClients() {
  StaticJsonDocument<256> jsonDoc;
  xSemaphoreTake(dataMutex, portMAX_DELAY);

  jsonDoc["pitch"] = telemetry.pitch;
  jsonDoc["roll"] = telemetry.roll;
  jsonDoc["yaw"] = telemetry.yaw;
  jsonDoc["temperature"] = telemetry.temperature;
  jsonDoc["humidity"] = telemetry.humidity;
  jsonDoc["battery"] = telemetry.batteryVoltage;

  xSemaphoreGive(dataMutex);

  String jsonStr;
  serializeJson(jsonDoc, jsonStr);
  ws.textAll(jsonStr);
  Serial.println(jsonStr);
}

// ------------------ WebSocket Events ------------------
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket Client Connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket Client Disconnected");
  }
}

// ------------------ Sensor Task ------------------
void sensorTask(void *parameter) {
  sensors_event_t a, g, temp;

  while (true) {
    mpu.getEvent(&a, &g, &temp);

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // Validate DHT readings
    if (isnan(t)) t = -1.0;
    if (isnan(h)) h = -1.0;

    float battADC = analogRead(BATTERY_PIN);
    float voltage = battADC * (3.3 / 4095.0) * 2; // Assume voltage divider

    float pitch = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
    float roll = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
    float yaw = g.gyro.z * 57.2958; // rough estimation

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    telemetry.pitch = pitch;
    telemetry.roll = roll;
    telemetry.yaw = yaw;
    telemetry.temperature = t;
    telemetry.humidity = h;
    telemetry.batteryVoltage = voltage;
    xSemaphoreGive(dataMutex);

    vTaskDelay(pdMS_TO_TICKS(100)); // 100 ms update rate
  }
}

// ------------------ Sender Task ------------------
void sendTask(void *parameter) {
  while (true) {
    notifyClients();
    vTaskDelay(pdMS_TO_TICKS(1000)); // Send every 1 second
  }
}

// ------------------ Setup ------------------
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected. IP address: " + WiFi.localIP().toString());

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  dht.begin();
  analogReadResolution(12);

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();

  dataMutex = xSemaphoreCreateMutex();

  // Create RTOS Tasks
  xTaskCreatePinnedToCore(sensorTask, "Sensor Task", 4096, NULL, 1, &sensorTaskHandle, 1);
  xTaskCreatePinnedToCore(sendTask, "Sender Task", 4096, NULL, 1, &sendTaskHandle, 1);
}

void loop() {
  // Empty, all logic handled by RTOS tasks
}

