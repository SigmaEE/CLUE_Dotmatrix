#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DEBUG 0

#if DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, ...)
#endif

IPAddress gateway(192, 168, 4, 1);
IPAddress local_ip_address(192, 168, 4, 2);
IPAddress udp_broadcast_address(192, 168, 4, 255);
IPAddress net_mask(255, 255, 255, 0);

const uint16_t udp_broadcast_port = 5578;
const uint16_t server_port = 8467;
const uint16_t crc16_polynomial = 0x1021;
const uint8_t transmission_start_identifier_length = 1;

const uint8_t animation_frames_message_identifier = 0x10;
const uint8_t game_of_life_config_message_identifier = 0x11;

enum response_code {
  RESPONSE_OK = 0x01,
  RESPONSE_INTERNAL_ERROR = 0x02,
  RESPONSE_CHECKSUM_MISMATCH = 0x03,
  RESPONSE_TIMEOUT = 0x04,
  RESPONSE_TARGET_UNEXPECTED_DATA = 0x05,
  RESPONSE_UNEXPECTED = 0x06
};

WiFiUDP udp;
WiFiServer server(local_ip_address, server_port);

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(3000);

  DEBUG_PRINTLN("Setting up device as access point...");  
  bool result = WiFi.softAPConfig(local_ip_address, gateway, net_mask);
  if (!result) {
    DEBUG_PRINTLN("Could not configure network parameters! This is unexpected");
    return;
  }

  result = WiFi.softAP("PinMatrixNetwork", "flipthedot");

  if (result) {
    DEBUG_PRINTLN("Network configured, access point live");
    DEBUG_PRINT("Local IP address: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
  }
  else {
    DEBUG_PRINTLN("Could not configure access point! This is unexpected");
  }

  server.begin();
  DEBUG_PRINTLN("Server started");
}

void send_udp_broadcast() {
  uint8_t broadcast_packet[] = { (uint8_t)((server_port & 0xff00) >> 8), (uint8_t)(server_port & 0x00ff), local_ip_address[0], local_ip_address[1], local_ip_address[2], local_ip_address[3] };
  DEBUG_PRINT("Sending UDP broadcast... ");
  udp.beginPacket(udp_broadcast_address, udp_broadcast_port);
  udp.write(broadcast_packet, 6);
  udp.endPacket();
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

response_code forward_transmission_header(uint8_t identifier, uint8_t* additional_data_buffer, uint8_t additional_data_length) {
  ensure_serial_input_buffer_empty();
  Serial.write(identifier);
  if (additional_data_length > 0)
    Serial.write(additional_data_buffer, additional_data_length);
  return get_response_from_target();
}

response_code forward_message(uint8_t* header, uint8_t header_length, uint8_t* data, uint8_t data_length) {
  ensure_serial_input_buffer_empty();
  Serial.write(header, header_length);
  Serial.write(data, data_length);
  return get_response_from_target();
}

void read_and_forward_message(WiFiClient& client, uint8_t header_length, uint16_t number_of_frames) {
  uint8_t message_header[header_length];
  uint16_t number_of_received_frames, crc, calculated_crc, payload_length;
  bool crc_match;
  response_code response_from_target;
  number_of_received_frames = 0;

  while (number_of_received_frames < number_of_frames) {

    while (!client.available())
      delay(50);

    if (!try_read_number_of_bytes_from_client(client, message_header, header_length)) {
      DEBUG_PRINTLN("Reading of message header failed");
      return;
    }

    crc = message_header[1] << 8 | message_header[0];
    payload_length = message_header[3] << 8 | message_header[2];

    DEBUG_PRINT("CRC: ");
    DEBUG_PRINTF("0x%02x\n", crc);
    DEBUG_PRINT("Payload length: ");
    DEBUG_PRINTLN(payload_length);

    uint8_t data_buffer[payload_length];
    if (!try_read_number_of_bytes_from_client(client, data_buffer, payload_length)) {
      DEBUG_PRINTF("Reading of data buffer %d data failed\n", number_of_received_frames);
      return;
    }

    calculated_crc = compute_crc16_checksum(data_buffer, payload_length);
    crc_match = calculated_crc == crc;
    DEBUG_PRINT("Computed CRC: ");
    DEBUG_PRINTF("0x%02x (%s)\n", calculated_crc, crc_match ? "match" : "mismatch");

    if (!crc_match) {
      send_response(client, response_code::RESPONSE_CHECKSUM_MISMATCH);
      delay(50);
      continue;
    }

    response_from_target = forward_message(message_header, 4, data_buffer, payload_length);
    send_response(client, response_from_target);
    if (response_from_target != response_code::RESPONSE_OK)
      break;

    number_of_received_frames++;
  }
}

void handle_incoming_transmission(WiFiClient& client) {
  uint8_t identifier_buffer[transmission_start_identifier_length];
  if (!try_read_number_of_bytes_from_client(client, identifier_buffer, transmission_start_identifier_length)) {
    DEBUG_PRINTLN("Reading of transmission identifier failed");
    return;
  }

  DEBUG_PRINTF("Message identifier: 0x%02X\n", identifier_buffer[0]);

  uint16_t number_of_frames;
  uint8_t header_length;
  response_code code;

  if (identifier_buffer[0] == animation_frames_message_identifier) {
    uint8_t additional_header_fields[6];
    if (!try_read_number_of_bytes_from_client(client, additional_header_fields, 6)) {
      DEBUG_PRINTLN("Reading of additional fields in header failed");
      return;
    }

    number_of_frames = additional_header_fields[1] << 8 | additional_header_fields[0];
    DEBUG_PRINTF("Number of frames: %d\n", number_of_frames);
    DEBUG_PRINTF("Number of bytes per frame: %d\n", additional_header_fields[3] << 8 | additional_header_fields[2]);
    DEBUG_PRINTF("Number of rows: %d\n", additional_header_fields[4]);
    DEBUG_PRINTF("Number of columns: %d\n", additional_header_fields[5]);

    code = forward_transmission_header(animation_frames_message_identifier, additional_header_fields, 6);
    header_length = 4;
  }
  else if (identifier_buffer[0] == game_of_life_config_message_identifier) {
    code = forward_transmission_header(game_of_life_config_message_identifier, NULL, 0);
    header_length = 4;
    number_of_frames = 1;
  }
  else {
    DEBUG_PRINTF("Got unknown message identifier (0x%02x)\n", identifier_buffer[0]);
    return;
  }

  send_response(client, code);
  if (code != response_code::RESPONSE_OK) {
    DEBUG_PRINTF("Target did not accept transmission header (response_code: %d)\n", (uint8_t)code);
    return;
  }

  read_and_forward_message(client, header_length, number_of_frames);
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
      }
      delay(5000);
    }
  }
  else {
    DEBUG_PRINTLN("No client connected to access point");
    delay(5000);
  }
}