#include "WiFi.h"
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "ESP_Color.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String lightStatus = "off";
void toggleOnOff() {
  if (lightsOn) {
    prevLedMode = ledMode;
    ledMode = 2;
    lightsOn = 0;
    lightStatus = "off";
  } else {
    ledMode = prevLedMode;
    lightsOn = 1;
    lightStatus = "on";
  }
  ws.textAll(lightStatus);
}

void startSleep() {
  Serial.println("starting sleep");
  WiFi.disconnect();
  server.end();
  //setCpuFrequencyMhz(80);
  sleeping = true;
  Serial.println("sleep started");
}

void endSleep() {
  Serial.println("ending sleep");
  setupWiFi();
  server.begin();
  //setCpuFrequencyMhz(80);
  sleeping = false;
  Serial.println("sleep ended");
}

// Replaces placeholder with LED state value
String processor(const String& var){
  return "";
  if (lightsOn) {
    return "on";
  } else {
    return "off";
  }
}

String getBatteryVoltage() {
  int batV = 0;
  for(int i = 0; i < 16; i++) {
    batV = batV + analogReadMilliVolts(A3); // ADC with correction   
  }
  float batVCalcd = 2.0 * float(batV) / 16.0 / 1000.0;     // attenuation ratio 1/2, mV --> V

  return "V" + String(batVCalcd, 2);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String str = (char*)data;
    if (str == "toggle") {
      toggleOnOff();
    } else if (str == "white") {
      ledMode = 0;
      lightsOn = 1;
      ws.textAll("on");
    } else if (str == "hue") {
      ledMode = 1;
      lightsOn = 1;
      ws.textAll("on");
    } else if (str == "rainbow") {
      ledMode = 3;
      lightsOn = 1;
      ws.textAll("on");
    } else if (str == "pulse") {
      calibrateMpu();
      ledMode = 4;
      lightsOn = 1;
      ws.textAll("on");
    } else if (str.substring(0, 1) == "#") { // if hue value {
      c = ESP_Color::Color::FromHex(str);
    } else { // must be brightness value
      brightness = str.toInt();
    }
  }
}

void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      lightStatus = (lightsOn) ? "on" : "off";
      ws.textAll(lightStatus);
      ws.textAll(String((int)brightness));
      ws.textAll(getBatteryVoltage());
      connectedClients = ws.count();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      connectedClients = ws.count();
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      Serial.println("Web Socket Error!");
      break;
  }
}

void initWebSocket() {
  Serial.println("Initializing WebSocket.");
  ws.onEvent(onWSEvent);
  server.addHandler(&ws);
}
 
void startWebServer() {
  Serial.println("Starting Web Server");
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    return;
  }

  Serial.println("Starting mDNS.");
  if(!MDNS.begin("dartboard")) {
     Serial.println("Error starting mDNS");
     return;
  }

  initWebSocket();  

  // Route for index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  // Start server
  server.begin();
}

void handleWS(void *parameter) {
  for(;;) {
    ws.cleanupClients();
    connectedClients = ws.count();
    if (connectedClients > 0) {
      //ws.textAll(getBatteryVoltage());
    }
    vTaskDelay(1000);
  }
}
