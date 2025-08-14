#define BLYNK_DEVICE_NAME "carbonaware"
#define BLYNK_TEMPLATE_NAME "carbonaware"
#define BLYNK_AUTH_TOKEN "KJ1OIBmXzFgJ0Sy5Zh9jWjuiEfnvXSC7"
#define BLYNK_TEMPLATE_ID "TMPL341t1xoL6"

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BlynkSimpleEsp32.h>

// Pins
#define MQ135_PIN 34 
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

Adafruit_MPU6050 mpu;

// Wi-Fi & Blynk credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Tania's Galaxy";
char pass[] = "tbgn0693";

// Tracking variables
float accumulatedPoints = 0;
String lastActivity = "";
unsigned long lastMillis = 0;
const unsigned long interval = 1000; // 1s updates

// Air quality filter
float filteredAirQuality = 0;
const float alpha = 0.1;

// Classify activity from acceleration magnitude
String classifyActivity(float magnitude) {
  if (magnitude < 0.5) return "Stationary";
  if (magnitude < 2.0) return "Walking";
  if (magnitude < 5.0) return "Biking";
  return "Driving";
}

// Carbon points based on activity
float getCarbonPoints(String activity) {
  if (activity == "Walking" || activity == "Biking") return 4.0;
  if (activity == "Driving") return -1.0;
  return 0.0;
}

// RGB LED indication for air quality
void controlLED(int airQuality) {
  if (airQuality < 300) {       // Good
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    Blynk.virtualWrite(V3, 0);
  } 
  else if (airQuality < 600) {  // Moderate
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    Blynk.virtualWrite(V3, 1);
  } 
  else {                        // Poor
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
    Blynk.virtualWrite(V3, 2);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(MQ135_PIN, INPUT);

  if (!mpu.begin()) {
    Serial.println("MPU6050 init failed");
    while (1);
  }

  Blynk.begin(auth, ssid, pass);
  Serial.println("Setup complete");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastMillis >= interval) {
    lastMillis = currentMillis;

    // Read & filter air quality
    int airQuality = analogRead(MQ135_PIN);
    filteredAirQuality = alpha * airQuality + (1 - alpha) * filteredAirQuality;
    int mappedAirQuality = map(filteredAirQuality, 0, 1023, 0, 100);
    Serial.println("Air Quality: " + String(mappedAirQuality));
    controlLED(mappedAirQuality);
    Blynk.virtualWrite(V0, mappedAirQuality);

    // Read acceleration
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float magnitude = sqrt(pow(a.acceleration.x, 2) + 
                            pow(a.acceleration.y, 2) + 
                            pow(a.acceleration.z, 2)) - 9.8;
    if (magnitude < 0) magnitude = 0;

    Serial.printf("X: %.2f Y: %.2f Z: %.2f Magnitude: %.2f\n",
                  a.acceleration.x, a.acceleration.y, a.acceleration.z, magnitude);

    String activity = classifyActivity(magnitude);

    if (activity != lastActivity) {
      accumulatedPoints += getCarbonPoints(activity);
      lastActivity = activity;
    }

    Serial.println("Activity: " + activity);
    Serial.println("Carbon Points: " + String(accumulatedPoints));

    Blynk.virtualWrite(V1, activity);
    Blynk.virtualWrite(V2, accumulatedPoints);
  }

  Blynk.run();
}
