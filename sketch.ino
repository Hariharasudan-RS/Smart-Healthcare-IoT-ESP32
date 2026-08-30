#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "AdafruitIO_WiFi.h"
#include <esp_task_wdt.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""

#define IO_USERNAME "hariharasudan"
#define IO_KEY ""

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

AdafruitIO_Feed *heartRate = io.feed("heart-rate");
AdafruitIO_Feed *spo2 = io.feed("spo2");
AdafruitIO_Feed *bodyTemp = io.feed("body-temp");

AdafruitIO_Feed *roomTemp = io.feed("room-temp");
AdafruitIO_Feed *oxygenLevel = io.feed("oxygen-level");
AdafruitIO_Feed *aqiFeed = io.feed("aqi");

AdafruitIO_Feed *dosageSlider = io.feed("dosage-slider");
AdafruitIO_Feed *bedSlider = io.feed("bed-slider");
AdafruitIO_Feed *samplingSlider = io.feed("sampling-rate");

AdafruitIO_Feed *dosageConfirm = io.feed("dosage-confirm");

// OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pins
#define SERVO_PIN 18
#define LED_PIN 19
#define BUZZER_PIN 23
#define AQI_PIN 35
#define OXYGEN_PIN 34

Servo bedServo;

// Shared Variables
int aqi = 0;
float oxygen = 0;
int servoAngle = 0;   // Sensor value
int targetServoAngle = 0;
int bedAngle = 0;     // Bed slider value
int manualSamplingRate = 1500;
SemaphoreHandle_t dataMutex;
QueueHandle_t sensorQueue;
QueueHandle_t offlineQueue;

struct SensorData
{
  int aqi;
  float oxygen;
};

struct OfflineData
{
    int aqi;
    float oxygen;
    int dosage;
    int bedAngle;
};

int dosageValue = 0;

int targetDosage = 0;      // Value received from dashboard
bool dosageConfirmed = false;
bool waitingConfirmation = false;

int samplingRate = 300;
bool autoSampling = true;
bool manualSampling = false;
bool offlineMode = false;

enum SystemState
{
  ONLINE,
  DEGRADED,
  OFFLINE
};

SystemState systemState = OFFLINE;

//---------------- SENSOR TASK ----------------//
//---------------- SENSOR TASK ----------------//
void SensorTask(void *pvParameters)
{
  //esp_task_wdt_add(NULL);
  while (1)
  {
    int aqiRaw = analogRead(AQI_PIN);
    int oxyRaw = analogRead(OXYGEN_PIN);

    SensorData data;

    data.aqi = map(aqiRaw, 0, 4095, 0, 500);
    data.oxygen = map(oxyRaw, 0, 4095, 180, 230) / 10.0;
    xSemaphoreTake(dataMutex, portMAX_DELAY);

    aqi = data.aqi;
    oxygen = data.oxygen;

    xSemaphoreGive(dataMutex);
    xQueueSend(sensorQueue, &data, portMAX_DELAY);

    if (systemState != ONLINE)
{
    OfflineData offlineData;

    offlineData.aqi = data.aqi;
    offlineData.oxygen = data.oxygen;
    offlineData.dosage = dosageValue;
    offlineData.bedAngle = targetServoAngle;

    if (xQueueSend(offlineQueue, &offlineData, 0) == pdTRUE)
    {
        Serial.println("Data stored in offline buffer");
    }
    else
    {
        Serial.println("Offline buffer FULL");
    }
}

    Serial.print("AQI = ");
    Serial.print(aqi);
    Serial.print(" Oxygen = ");
    Serial.println(oxygen);

    //esp_task_wdt_reset();
    if (autoSampling)
{
    if (aqi > 150 || oxygen < 18.0 || dosageValue > 80)
{
    samplingRate = 500;      // Emergency monitoring
}
else
{
    samplingRate = manualSamplingRate;   // Doctor's selected rate
}
}
Serial.print("Manual Rate = ");
Serial.print(manualSamplingRate);

Serial.print(" | Current Rate = ");
Serial.println(samplingRate);

vTaskDelay(pdMS_TO_TICKS(samplingRate));
  }
}

//---------------- SERVO TASK ----------------//
void ServoTask(void *pvParameters)
{
  while (1)
  {
    if (servoAngle < targetServoAngle)
    {
      servoAngle++;

      if (servoAngle > targetServoAngle)
        servoAngle = targetServoAngle;
    }
    else if (servoAngle > targetServoAngle)
    {
      servoAngle--;

      if (servoAngle < targetServoAngle)
        servoAngle = targetServoAngle;
    }

    bedServo.write(servoAngle);

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void OLEDTask(void *pvParameters)
{
  SensorData receivedData;
  
  while (1)
  {
      if (xQueueReceive(sensorQueue, &receivedData, portMAX_DELAY))
  {
      aqi = receivedData.aqi;
      oxygen = receivedData.oxygen;
      
  }

    xSemaphoreTake(dataMutex, portMAX_DELAY);

    display.clearDisplay();

    if (offlineMode)
    {
      display.setCursor(0,0);
      display.println("LOGGING");
      display.println("OFFLINE");
      display.display();

      xSemaphoreGive(dataMutex);

      vTaskDelay(pdMS_TO_TICKS(500));

      continue;
    }

    display.setCursor(0,0);
    display.print("AQI : ");
    display.println(aqi);

    display.print("Oxygen : ");
    display.print(oxygen);
    display.println("%");

    display.print("Bed : ");
display.print(servoAngle);
display.println((char)247);

display.print("Dosage : ");
display.println(dosageValue);

display.print("Sample: ");
display.print(samplingRate);
display.println(" ms");

display.print("Status: ");

if (systemState == ONLINE)
{
  display.println("ONLINE");
}
else if (systemState == DEGRADED)
{
  display.println("DEGRADED");
}
else
{
  display.println("OFFLINE");
}

if (servoAngle <= 15)
{
  display.println("SLEEP");
}
else if (servoAngle >= 35 && servoAngle <= 55)
{
  display.println("BREATHING");
}
else if (servoAngle >= 80)
{
  display.println("EMERGENCY");
}
else
{
  display.println("MANUAL");
}

  if(waitingConfirmation)
  {
    display.println();display.print("Bed : ");
  display.print(servoAngle);
  display.println((char)247);   // Degree symbol

  if (servoAngle <= 15)
  {
    display.println("Mode : SLEEP");
  }
  else if (servoAngle >= 35 && servoAngle <= 55)
  {
    display.println("Mode : BREATHING");
  }
  else if (servoAngle >= 80)
  {
    display.println("Mode : EMERGENCY");
  }
  else
  {
    display.println("Mode : MANUAL");
  }
    display.println("WAITING");
    display.println("FOR");
    display.println("CONFIRMATION");
  }
  else
  {
    if(aqi > 150)
      display.println("AIR QUALITY ALERT");
    else
      display.println("STATUS : NORMAL");
  }

      display.display();

      xSemaphoreGive(dataMutex);

        
      vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void AlertTask(void *pvParameters)
{
  
  while (1)
  {
    if (offlineMode)
    {
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 1500);

      vTaskDelay(pdMS_TO_TICKS(300));

      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);

      vTaskDelay(pdMS_TO_TICKS(300));
    }
    else if (aqi > 150 || dosageValue > 80)
    {
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 1000);

      vTaskDelay(pdMS_TO_TICKS(100));

      digitalWrite(LED_PIN, LOW);

      vTaskDelay(pdMS_TO_TICKS(100));
    }
    else
    {
      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);

        
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void MQTTTask(void *pvParameters)
{
  
  while (1)
  {
    heartRate->save(random(70,90));
    spo2->save(random(95,100));
    bodyTemp->save(random(360,380) / 10.0);
    roomTemp->save(random(240,320) / 10.0);
    oxygenLevel->save(oxygen);
    aqiFeed->save(aqi);

      
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void handleDosage(AdafruitIO_Data *data)
{
  Serial.println("===== DOSAGE CALLBACK =====");

  targetDosage = data->toInt();

  Serial.print("Target Dosage = ");
  Serial.println(targetDosage);

  if(targetDosage > 80)
  {
    waitingConfirmation = true;
  }
  else
  {
    waitingConfirmation = false;
    dosageConfirmed = true;
  }
}

void handleConfirm(AdafruitIO_Data *data)
{
  if (data->toInt() == 1)
  {
    Serial.println("Dosage Confirmed");

    waitingConfirmation = false;
    dosageConfirmed = true;
  }
  else
  {
    dosageConfirmed = false;
  }
}

void handleBed(AdafruitIO_Data *data)
{
  targetServoAngle = data->toInt();

  Serial.print("Bed Slider = ");
  Serial.println(targetServoAngle);
}

void handleSampling(AdafruitIO_Data *data)
{
    manualSamplingRate = data->toInt();

Serial.print("Manual Sampling = ");
Serial.println(manualSamplingRate);   
    
}

void DosageTask(void *pvParameters)
{
  while (1)
  {
    if (!waitingConfirmation)
    {
      if (dosageValue < targetDosage)
      {
        dosageValue += 5;

        if (dosageValue > targetDosage)
          dosageValue = targetDosage;
      }
      else if (dosageValue > targetDosage)
      {
        dosageValue -= 5;

        if (dosageValue < targetDosage)
          dosageValue = targetDosage;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void syncOfflineData()
{
  OfflineData offlineData;

    Serial.println("===== OFFLINE DATA SYNC =====");

      while (xQueueReceive(offlineQueue, &offlineData, 0) == pdTRUE)
        {
            Serial.print("Sync AQI: ");
                Serial.println(offlineData.aqi);

                    Serial.print("Sync Oxygen: ");
                        Serial.println(offlineData.oxygen);

                            Serial.print("Sync Dosage: ");
                                Serial.println(offlineData.dosage);

                                    Serial.print("Sync Bed Angle: ");
                                        Serial.println(offlineData.bedAngle);

                                            aqiFeed->save(offlineData.aqi);
                                                oxygenLevel->save(offlineData.oxygen);
                                                    dosageSlider->save(offlineData.dosage);
                                                        bedSlider->save(offlineData.bedAngle);

                                                            vTaskDelay(pdMS_TO_TICKS(500));
                                                              }

                                                                Serial.println("===== OFFLINE DATA SYNC COMPLETE =====");
                                                                }

void NetworkTask(void *pvParameters)
{
  
  while (1)
  {
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (io.status() != AIO_CONNECTED)
{
    offlineMode = true;

    if (WiFi.status() != WL_CONNECTED)
    {
      systemState = OFFLINE;
    }
    else
    {
      systemState = DEGRADED;
    }

      Serial.println("Connecting to Adafruit IO...");

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("ERROR: Wi-Fi OFFLINE");
    }
    else
    {
      Serial.println("ERROR: MQTT/Adafruit IO OFFLINE");
    }

      io.connect();

      int retry = 0;
      int retryDelay = 1000;

      while (io.status() < AIO_CONNECTED && retry < 5)
      {
        Serial.print(".");

          vTaskDelay(pdMS_TO_TICKS(retryDelay));

            retry++;

              retryDelay = retryDelay * 2;

                if (retryDelay > 8000)
                  {
                      retryDelay = 8000;
                        }
                        }
    if (io.status() != AIO_CONNECTED)
    {
      Serial.println();
        Serial.println("Adafruit IO connection failed.");
          Serial.println("Retrying later...");
            systemState = OFFLINE;
            }

      if (io.status() == AIO_CONNECTED)
{
    Serial.println();
    Serial.println("Adafruit IO Connected!");

    dosageSlider->onMessage(handleDosage);
    dosageConfirm->onMessage(handleConfirm);
    bedSlider->onMessage(handleBed);
    samplingSlider->onMessage(handleSampling);

    dosageSlider->get();
    dosageConfirm->get();
    bedSlider->get();
    samplingSlider->get();

    offlineMode = false;
    systemState = ONLINE;

    Serial.println("SYSTEM STATUS: ONLINE");

    syncOfflineData();
}
    }
    else
{
    offlineMode = false;
    systemState = ONLINE;
}

    io.run();

      
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  bedServo.attach(SERVO_PIN);

  dataMutex = xSemaphoreCreateMutex();
  sensorQueue = xQueueCreate(5, sizeof(SensorData));
  offlineQueue = xQueueCreate(20, sizeof(OfflineData));

  // OLED Initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED Failed");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(10, 20);
  display.println("SMART HEALTHCARE");
  display.setCursor(25, 35);
  display.println("SYSTEM READY");
  display.display();

  delay(2000);

  // ===========================
  // Connect to Adafruit IO
  // ===========================
  
  // ===========================
  // Create FreeRTOS Tasks
  // ===========================

  xTaskCreate(
      SensorTask,
      "SensorTask",
      4096,
      NULL,
      3,
      NULL);

  xTaskCreate(
      ServoTask,
      "ServoTask",
      2048,
      NULL,
      2,
      NULL);

  xTaskCreate(
      OLEDTask,
      "OLEDTask",
      4096,
      NULL,
      2,
      NULL);

  xTaskCreate(
      AlertTask,
      "AlertTask",
      2048,
      NULL,
      2,
      NULL);

  xTaskCreate(
    MQTTTask,
    "MQTTTask",
    4096,
    NULL,
    2,
    NULL);

  xTaskCreate(
    NetworkTask,
    "NetworkTask",
    4096,
    NULL,
    3,
    NULL);

  xTaskCreate(
    DosageTask,
    "DosageTask",
    2048,
    NULL,
    2,
    NULL
);
  
}
void loop()
{
  // FreeRTOS handles everything
   while(true);
}