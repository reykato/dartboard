#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "ESP_Color.h"
#include <Adafruit_NeoPixel.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"
#include <I2Cdev.h>

#define LED_COUNT 60
#define LED_PIN D9
#define INTERRUPT_PIN D6

#define WIFI_SSID    "Verizon_6NSP4Q"
#define WIFI_PASS    "splash9-fax-con"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP_Color::Color c;
String hexColor = "";
unsigned int ledMode = 0;
unsigned int prevLedMode = 0;
bool lightsOn = 1;
String hueString = "";

int brightness = 40;
int brightnessLast = 40;
int avgZValue = 0;
int reactiveThreshold = 600;

unsigned int connectedClients = 0;
unsigned int lastConnectedClients = 0;

unsigned int prevReactiveMillis = 0;
unsigned int timeLastHit = 0;
unsigned int sleepInterval = 10000; // time before shutting off wifi for power savings (ms)
bool sleeping = false;

// MPU6050 stuff
MPU6050 mpu;
VectorInt16 aa;         // [x, y, z]            accel sensor measurements

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer


// ====================================================================================================================================

int brightnessCalc(int maxBrightness) {
  return maxBrightness * (brightness/255.0f);
}

void setAllWhite() {
  strip.fill(strip.Color(255, 255, 255));
  strip.setBrightness(brightnessCalc(185));
}

void hueChange() {
  strip.fill(strip.Color(c.R_Byte(), c.G_Byte(), c.B_Byte()));
  strip.setBrightness(brightness);
}

unsigned int prevRainbow = 0;
long firstPixelHue = 0;

void rainbow() {
  strip.setBrightness(brightness);
  if (millis() - prevRainbow >= 10) {
    if (firstPixelHue < 5*65536) {
      strip.rainbow(firstPixelHue);
      strip.show(); // Update strip with new contents
      firstPixelHue += 256;
    } else {
      firstPixelHue = 0;
    }
    prevRainbow = millis();
  }
}

uint8_t reactiveBrightness = 220;
void reactive() {
  //Serial.println("Acceleration: " + String(aa.z) + ", Average: " + String(avgZValue));
  if (abs(aa.z - (avgZValue/20)) > reactiveThreshold) {
    timeLastHit = millis();
    reactiveBrightness = 220;
    Serial.println("Acceleration: " + String(aa.z) + ", pulsing.");
    strip.fill(strip.Color(c.R_Byte(), c.G_Byte(), c.B_Byte()));
  }
  if (millis() - prevReactiveMillis >= 10) {
    if (reactiveBrightness - 3 >= 0) {
      reactiveBrightness -= 3;
    } else {
      reactiveBrightness = 0;
    }
    strip.setBrightness(reactiveBrightness);
    prevReactiveMillis = millis();
  }
}

void off() {
  strip.clear();
}

void calibrateMpu() {
  avgZValue = 0;
  for (int i = 0; i < 20; i++) {
    Wire.beginTransmission(0x68);
    Wire.write(0x3B);  
    Wire.endTransmission(false);
    Wire.requestFrom(0x68,6,true);
    aa.x = Wire.read()<<8|Wire.read();
    aa.y = Wire.read()<<8|Wire.read();
    aa.z = Wire.read()<<8|Wire.read();
    avgZValue += aa.z;
  }
}

void setupWiFi() {
  //Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  //Serial.println("Connected! Starting OTA.");
  //Serial.println("Finished booting. :D");
}

void setupMpu() {
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);     
  Wire.endTransmission(true);
  calibrateMpu();
}

void mpuLoop()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);  
  Wire.endTransmission(false);
  Wire.requestFrom(0x68,6,true);
  aa.x = Wire.read()<<8|Wire.read();
  aa.y = Wire.read()<<8|Wire.read();
  aa.z = Wire.read()<<8|Wire.read();
}

// ====================================================================================================================================

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("//Serial Initialized");
  pinMode(LED_PIN, OUTPUT);
  pinMode(INTERRUPT_PIN, INPUT);
  delay(1000);// sanity check delay - allows reprogramming if accidently blowing power w/leds
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP  
  strip.setBrightness(brightness);
  
  setupWiFi();
  startWebServer();
  
  xTaskCreate(
    handleWS,
    "websocket",
    10000,
    NULL,
    0,
    NULL);  

  setupMpu();
  //esp_sleep_enable_timer_wakeup(1000000); // 1 million microseconds divided by 100 = 10ms
}

void loop() {  
  mpuLoop();
  if (millis() - timeLastHit >= sleepInterval) {
    if (!sleeping) {
      startSleep();
    }
  } else {
    if (sleeping) {
      endSleep();
    }
  }
  
  if (sleeping) {
    //esp_light_sleep_start();
    delay(4);
  } else {
    switch (ledMode) {
    case 0:
      setAllWhite();
      break;
    case 1:
      hueChange();
      break;
    case 2:
      off();
      break;
    case 3:
      rainbow();
      break;
    case 4:
      reactive();
      break;
    default:
      ledMode = 0;
      break;
    }
    strip.show();
    delay(4);
  }
}
