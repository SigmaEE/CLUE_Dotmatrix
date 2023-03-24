#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DEBUG 0
#define STA_MODE 0

#if DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, ...)
#endif

const uint16_t udp_broadcast_port = 5578;
const uint16_t server_port = 8467;
const uint16_t crc16_polynomial = 0x1021;
const uint8_t transmission_header_number_of_bytes = 3;
const uint8_t packet_header_number_of_bytes = 4;
const uint8_t connection_ping_id = 0xff;

const char* ssid = "sigma-guest";
const char* password = "starforlife2005";
const char* ap_name = "PinMatrixNetwork";
const char* ap_password = "flipthedot";

enum response_code {
  RESPONSE_OK = 0x01,
  RESPONSE_INTERNAL_ERROR = 0x02,
  RESPONSE_CHECKSUM_MISMATCH = 0x03,
  RESPONSE_TIMEOUT = 0x04,
  RESPONSE_TARGET_UNEXPECTED_DATA = 0x05,
  RESPONSE_UNEXPECTED = 0x06
};

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(3000);

#if STA_MODE
  if (!WiFi.mode(WIFI_STA)) {
    DEBUG_PRINTLN("Could not set module to WiFi Station mode! This is unexpected");
    return;
  }
  DEBUG_PRINTF("Trying to connect to network '%s'...", ssid);
  WiFi.begin(ssid, password);
#else
  if (!WiFi.mode(WIFI_AP) || ! WiFi.softAP(ap_name, ap_password)) {
    DEBUG_PRINTLN("Could not set module to WiFi Access Point mode! This is unexpected");
    return;
  }
#endif
}

void send_udp_broadcast(WiFiUDP& udp, const IPAddress& local_ip_address, const IPAddress& broadcast_ip_address) {
  uint8_t broadcast_packet[] = {
    (uint8_t)((server_port & 0xff00) >> 8),
    (uint8_t)(server_port & 0x00ff),
    local_ip_address[0],
    local_ip_address[1],
    local_ip_address[2], local_ip_address[3] };

  DEBUG_PRINT("Sending UDP broadcast... ");
  if (udp.beginPacket(broadcast_ip_address, udp_broadcast_port) != 1)
    DEBUG_PRINTLN("WiFiUDP.beginPacket() failed");
  uint8_t write_result = udp.write(broadcast_packet, 6);

  if (write_result != 6)
    DEBUG_PRINTF("WiFiUDP.write() returned %d\n", write_result);

  if (udp.endPacket() != 1)
    DEBUG_PRINTLN("WiFiUDP.endPacket() failed");

  DEBUG_PRINTLN("Done.");
}

void send_response(WiFiClient& client, response_code response) {
  DEBUG_PRINTF("Sending response 0x%02x to client\n", response);
  client.write((uint8_t)response);
}

uint16_t compute_crc16_checksum(uint8_t* data, uint16_t length) {
  // See https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
  uint16_t crc = 0xffff;
  uint16_t i = 0;
  while (i < length) {
    crc ^= (data[i] << 8);
    for (uint8_t j = 0; j < 8; j++) {
      if ((crc & 0x8000) > 0)
        crc = (crc << 1) ^ crc16_polynomial;
      else
        crc = crc << 1;
    }
    i++;
  }
  return crc;
}

bool try_read_number_of_bytes_from_client(WiFiClient& client, uint8_t* buffer, uint16_t number_of_bytes) {
  uint16_t byte_ctr = 0;
  while (byte_ctr < number_of_bytes) {
    int result = client.read();
    if (result == -1)
      break;

    buffer[byte_ctr++] = (uint8_t)result;
  }

  if (number_of_bytes != byte_ctr) {
    DEBUG_PRINTF("Reading from client failed, got %d bytes\n", byte_ctr);
    for (uint8_t i = 0; i < byte_ctr; i++)
      DEBUG_PRINTF("0x%02x, ", buffer[i]);
    DEBUG_PRINTLN("");
  }
  return number_of_bytes == byte_ctr;
}

void ensure_serial_input_buffer_empty() {
  while (Serial.available() > 0)
    Serial.read();
}

response_code get_response_from_target() {
  uint8_t response_buffer[1];
  uint8_t number_of_read_bytes = Serial.readBytes(response_buffer, 1);
  if (number_of_read_bytes == 0)
    return response_code::RESPONSE_TIMEOUT;
  else if (number_of_read_bytes > 1 || response_buffer[0] > 0x06)
    return response_code::RESPONSE_UNEXPECTED;
  else
    return (response_code)response_buffer[0];
}

response_code forward_message(uint8_t* header, uint8_t header_length, uint8_t* data, uint8_t data_length) {
  ensure_serial_input_buffer_empty();
  Serial.write(header, header_length);
  if (data_length > 0)
    Serial.write(data, data_length);
  return get_response_from_target();
}

void read_and_forward_packet(WiFiClient& client, uint16_t number_of_packets) {
  uint8_t message_header[packet_header_number_of_bytes];
  uint16_t number_of_received_packets, crc, calculated_crc, payload_length;
  bool crc_match;
  response_code response_from_target;
  number_of_received_packets = 0;

  while (number_of_received_packets < number_of_packets) {

    while (!client.available())
      delay(50);

    if (!try_read_number_of_bytes_from_client(client, message_header, packet_header_number_of_bytes)) {
      DEBUG_PRINTLN("Reading of message header failed");
      return;
    }

    crc = message_header[1] << 8 | message_header[0];
    payload_length = message_header[3] << 8 | message_header[2];

    DEBUG_PRINTF("Packet %d/%d\n", number_of_received_packets + 1, number_of_packets);
    DEBUG_PRINTF("   CRC: 0x%02x\n", crc);
    DEBUG_PRINTF("   Payload length: %d\n", payload_length);

    uint8_t data_buffer[payload_length];
    if (!try_read_number_of_bytes_from_client(client, data_buffer, payload_length)) {
      DEBUG_PRINTF("Reading of data packet %d failed\n", number_of_received_packets);
      return;
    }

    calculated_crc = compute_crc16_checksum(data_buffer, payload_length);
    crc_match = calculated_crc == crc;
    DEBUG_PRINTF("   Computed CRC:  0x%02x (%s)\n", calculated_crc, crc_match ? "match" : "mismatch");

    if (!crc_match) {
      send_response(client, response_code::RESPONSE_CHECKSUM_MISMATCH);
      delay(50);
      continue;
    }

    response_from_target = forward_message(message_header, packet_header_number_of_bytes, data_buffer, payload_length);
    send_response(client, response_from_target);
    if (response_from_target != response_code::RESPONSE_OK)
      break;

    number_of_received_packets++;
  }
}

void handle_incoming_transmission(WiFiClient& client) {
  uint8_t transmission_header_buffer[transmission_header_number_of_bytes];
  if (!try_read_number_of_bytes_from_client(client, transmission_header_buffer, transmission_header_number_of_bytes)) {
    DEBUG_PRINTLN("Reading of transmission header failed");
    return;
  }

  if (transmission_header_buffer[0] == connection_ping_id) {
    send_response(client, response_code::RESPONSE_OK);
    return;
  }

  uint16_t number_of_packets = transmission_header_buffer[2] << 8 | transmission_header_buffer[1];
  response_code code = forward_message(transmission_header_buffer, transmission_header_number_of_bytes, nullptr, 0);

  DEBUG_PRINTF("Received message with id 0x%02x, number of packet: %d\n", transmission_header_buffer[0], number_of_packets);

  send_response(client, code);
  if (code != response_code::RESPONSE_OK) {
    DEBUG_PRINTF("Target did not accept transmission header (response_code: %d)\n", (uint8_t)code);
    return;
  }

  read_and_forward_packet(client, number_of_packets);
}

bool get_broadcast_condition(bool has_connected_client) {
#if STA_MODE
  return WiFi.status() == WL_CONNECTED && !has_connected_client;
#else
  return !has_connected_client;
#endif
}

void loop() {
  static bool has_connected_client = false;
  static WiFiServer server(server_port);
  static WiFiUDP udp;
  static IPAddress ip_address;
  static bool broadcast_condition;

#if STA_MODE
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN(" connected!");
#endif

#if STA_MODE
  ip_address = WiFi.localIP();
#else
  ip_address = WiFi.softAPIP();
#endif

  IPAddress broadcast_address = ip_address;
  broadcast_address[3] = 0xff;
#if STA_MODE
  DEBUG_PRINTF("SSID: %s\nHost name: %s\nIP address: %s\nSubnet mask: %s\nGateway IP: %s\n",
    WiFi.SSID().c_str(), WiFi.hostname().c_str(), ip_address.toString().c_str(), WiFi.subnetMask().toString().c_str(), WiFi.gatewayIP().toString().c_str());
#else
DEBUG_PRINTF("SSID: %s\nHost name: %s\nIP address: %s\n",
    WiFi.SSID().c_str(), WiFi.hostname().c_str(), ip_address.toString().c_str());
#endif

  DEBUG_PRINTF("Broadcast target: %s:%d\n", broadcast_address.toString().c_str(), udp_broadcast_port); 

  server.begin();
  DEBUG_PRINTLN("Server started");

  WiFiClient client;
  while (get_broadcast_condition(has_connected_client)) {
    send_udp_broadcast(udp, ip_address, broadcast_address);
    client = server.accept();
    if (client)
      has_connected_client = true;
    else
      delay(5000);
  }

  DEBUG_PRINTLN("Client connected");
  while (client.connected()) {
    if (client.available()) {
      DEBUG_PRINTLN("Data available from client");
      handle_incoming_transmission(client);
    }
    else {
      delay(50);
    }
  }
  has_connected_client = false;
}