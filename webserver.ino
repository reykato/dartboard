#include "WiFiUDP.h"

IPAddress UDP_ADDRESS = IPAddress(192,168,1,163);
#define UDP_PORT 6969

WiFiUDP udp;

void initUDP() {
  Serial.println("Starting UDP Client.");
  udp.begin(UDP_PORT);
  ws.textAll("Connected to UDP");
}
void sendUDP() {
    udp.beginPacket(UDP_ADDRESS, UDP_PORT);
    udp.printf("DB_HIT");
    
    udp.endPacket();
}
void sendUDP(String str) {
    udp.beginPacket(UDP_ADDRESS, UDP_PORT);
    udp.printf(str.c_str());
    
    udp.endPacket();
}
void maint(void* pvParameters) {
  for(;;) {
    sendUDP(" ");
    delay(10);
  }
}
