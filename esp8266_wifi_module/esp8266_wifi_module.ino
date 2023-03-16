#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DEBUG 1

IPAddress gateway(192, 168, 4, 1);
IPAddress local_ip_address(192, 168, 4, 2);
IPAddress udp_broadcast_address(192, 168, 4, 255);
IPAddress net_mask(255, 255, 255, 0);

const uint16_t udp_broadcast_port = 5578;
const uint16_t server_port = 8467;

WiFiUDP udp;
WiFiServer server(local_ip_address, server_port);

void setup() {
#if DEBUG
  Serial.begin(115200);
  Serial.println("Setting up device as access point...");
#endif
  
  bool result = WiFi.softAPConfig(local_ip_address, gateway, net_mask);
  if (!result) {
#if DEBUG
    Serial.println("Could not configure network parameters! This is unexpected");
#endif
    return;
  }

  result = WiFi.softAP("PinMatrixNetwork", "flipthedot");

#if DEBUG
  if (result) {
    Serial.println("Network configured, access point live");
    Serial.print("Local IP address: ");
    Serial.println(WiFi.softAPIP());
  }
  else {
    Serial.println("Could not configure access point! This is unexpected");
  }
#endif

  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

}

void send_udp_broadcast() {
  uint8_t broadcast_packet[] = { (uint8_t)((server_port & 0xff00) >> 8), (uint8_t)(server_port & 0x00ff), local_ip_address[0], local_ip_address[1], local_ip_address[2], local_ip_address[3] };
#if DEBUG
  Serial.print("No connected clients, sending UDP broadcast... ");
#endif
  udp.beginPacket(udp_broadcast_address, udp_broadcast_port);
  udp.write(broadcast_packet, 6);
  udp.endPacket();
#if DEBUG
  Serial.println("Done.");
#endif  
}

void loop() {
  static int number_of_connected_stations = 0;
  static bool has_connected_client = false;

  number_of_connected_stations = WiFi.softAPgetStationNum();
  if (number_of_connected_stations > 0) {
    while (!has_connected_client) {
      send_udp_broadcast();
      WiFiClient client = server.accept();
      if (client) {
        Serial.println("Client connected");
      }
      else {
        Serial.println("No client connected before timeout");
      }
      delay(5000);
    }
  }
  else {
    Serial.println("No stations connected");
    delay(5000);
  }
}