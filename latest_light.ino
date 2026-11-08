#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define TRIGGER_PIN 13
#define ECHO_PIN 12
#define NUM_READINGS 3
const int ledPin = 14;
#define LANTERN_ON HIGH
#define LANTERN_OFF LOW

//фильтр для сглаживания расстояния
#define FILTER_ALPHA 0.6 

//время удержания(мс)
#define HOLD_TIME_AFTER_CAR 5000

//Volatile переменные для обмена данными с прерыванием
volatile unsigned long startTime = 0;
volatile unsigned long endTime = 0;
volatile bool newDataAvailable = false;

//для управления фонарем
volatile bool statlight = false;
volatile unsigned long lightOnTime = 0;
volatile bool holdMode = false;
volatile unsigned long holdStartTime = 0;

//история измерений
int distancesCm[NUM_READINGS];
unsigned long readingTimesMs[NUM_READINGS];
int readingIndex = 0;
bool readingsFull = false;

//фильтрованное расстояние
float filteredDistance = 200.0;

//мьютекс для защиты общих переменных
SemaphoreHandle_t dataMutex;

//обработчик прерывания(ISR)
void IRAM_ATTR handleEchoInterrupt() {
  if (digitalRead(ECHO_PIN) == HIGH) {
    startTime = micros();
  } else {
    endTime = micros();
    newDataAvailable = true;
  }
}

//задача FreeRTOS для запуска измерений
void TaskMeasureDistance(void *pvParameters) {
  while (1) {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

//экспоненциальное сглаживание
float applyFilter(float newValue, float oldValue, float alpha) {
  return alpha * newValue + (1.0 - alpha) * oldValue;
}

//управление фонарем
void updateLightControl(int currentDistance) {
  unsigned long currentMillis = millis();
  
  //фильтрация расстояния
  filteredDistance = applyFilter(currentDistance, filteredDistance, FILTER_ALPHA);
  
  //проверка условий включения
  if (filteredDistance < 40 && filteredDistance > 0) {
    //машина обнаружена вблизи
    digitalWrite(ledPin, LANTERN_ON);
    statlight = true;
    lightOnTime = currentMillis;
    holdMode = false;
    
    //отладка через Serial
    Serial.print("Car detected! Distance: ");
    Serial.print((int)filteredDistance);
    Serial.println(" cm - Light ON");
  } 
  else {
    //машина уехала или не обнаружена
    if (statlight && !holdMode) {
      //режим удержания на 5 секунд
      holdMode = true;
      holdStartTime = currentMillis;
      Serial.println("Car left - Hold mode started (5 seconds)");
    }
    
    //истекло ли время удержания
    if (holdMode) {
      if (currentMillis - holdStartTime < HOLD_TIME_AFTER_CAR) {
        //в режиме удержания-фонарь горит
        digitalWrite(ledPin, LANTERN_ON);
        statlight = true;
      } else {
        digitalWrite(ledPin, LANTERN_OFF);
        statlight = false;
        holdMode = false;
        Serial.println("Hold time expired - Light OFF");
      }
    } else {
      digitalWrite(ledPin, LANTERN_OFF);
      statlight = false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Ultrasonic Light Control Started");
  Serial.println("System Ready");
  
  // Настройка пинов
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LANTERN_OFF);
  
  //создание мьютекса
  dataMutex = xSemaphoreCreateMutex();
  
  //настройка прерывания
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), handleEchoInterrupt, CHANGE);
  
  //задача измерения
  xTaskCreate(
    TaskMeasureDistance,
    "Measure Task",
    2048,
    NULL,
    1,
    NULL
  );
  
  Serial.println("Setup complete - Waiting for cars...");
}

void loop() {
  static unsigned long lastSerialPrint = 0;
  unsigned long currentMillis = millis();
  
  if (newDataAvailable) {
    if (xSemaphoreTake(dataMutex, (TickType_t) 10) == pdTRUE) {
      //обработка нового измерения
      unsigned long duration = endTime - startTime;
      int currentDistanceCm = (duration * 0.0343) / 2;
      
      //проверка на корректность измерения
      if (currentDistanceCm > 200 || currentDistanceCm < 0 || duration > 11650) {
        currentDistanceCm = 200;
      }
      
      newDataAvailable = false;
      
      //сохранение в историю
      distancesCm[readingIndex] = currentDistanceCm;
      readingTimesMs[readingIndex] = millis();
      
      readingIndex++;
      if (readingIndex >= NUM_READINGS) {
        readingIndex = 0;
        readingsFull = true;
      }
      
      //обновление управления фонарем
      updateLightControl(currentDistanceCm);
      
      xSemaphoreGive(dataMutex);
    }
  }
  
  //периодический вывод статуса в Serial
  if (currentMillis - lastSerialPrint >= 1000) {
    if (xSemaphoreTake(dataMutex, (TickType_t) 5) == pdTRUE) {
      Serial.print("Status - Distance: ");
      Serial.print((int)filteredDistance);
      Serial.print(" cm, Light: ");
      Serial.print(statlight ? "ON" : "OFF");
      
      if (holdMode && statlight) {
        unsigned long holdRemaining = HOLD_TIME_AFTER_CAR - (currentMillis - holdStartTime);
        Serial.print(", Hold remaining: ");
        Serial.print(holdRemaining / 1000);
        Serial.print(".");
        Serial.print((holdRemaining % 1000) / 100);
        Serial.print("s");
      }
      Serial.println();
      
      lastSerialPrint = currentMillis;
      xSemaphoreGive(dataMutex);
    }
  }
  
  //обработка таймеров удержания даже без новых данных
  if (holdMode && statlight) {
    if (xSemaphoreTake(dataMutex, (TickType_t) 5) == pdTRUE) {
      if (millis() - holdStartTime >= HOLD_TIME_AFTER_CAR) {
        digitalWrite(ledPin, LANTERN_OFF);
        statlight = false;
        holdMode = false;
        Serial.println("Timer check: Light OFF");
      }
      xSemaphoreGive(dataMutex);
    }
  }
  
  vTaskDelay(pdMS_TO_TICKS(20));
}