#pragma once

#include "SerialCommunicator.h"

class Message {
public:
  enum class Type {
    AnimationFrames,
    GameOfLifeConfig
  };

  virtual Type get_type() const = 0;

  virtual SerialCommunicator::ReadResult read_message(HardwareSerial&, SerialCommunicator&) = 0;

  const static uint8_t animation_frames_message_identifier = 0x10;
  const static uint8_t game_of_life_config_message_identifier = 0x11;

  virtual ~Message() {}
protected:
  const static uint16_t crc16_polynomial = 0x1021;

  static bool try_read_crc_and_payload_length(HardwareSerial& serial_interface, uint16_t* crc, uint16_t* payload_length) {
    uint8_t header_buffer[4];
    if (serial_interface.readBytes(header_buffer, 4) != 4)
      return false;

    *crc = header_buffer[1] << 8 | header_buffer[0];
    *payload_length = header_buffer[3] << 8 | header_buffer[2];
    return true;
  }

  static uint16_t compute_crc16_checksum(uint8_t* data, uint16_t length) {
    // See https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
    uint16_t crc = 0xffff;
    uint16_t i = 0;
    while (i < length) {
      crc ^= (data[i] << 8);
      for (uint8_t j = 0; j < 8; j++) {
        if ((crc & 0x8000) > 0)
          crc = (crc << 1) ^ Message::crc16_polynomial;
        else
          crc = crc << 1;
      }
      i++;
    }
    return crc;
  }
};

struct AnimationFrameHeader {
public:
  uint16_t index;
  uint16_t bytes_per_frame;
  uint8_t number_of_rows;
  uint8_t number_of_columns;
  uint16_t frame_time_ms;
  uint8_t number_of_animation_repeats;

  static const uint8_t header_length = 9;
};

class AnimationFrame {
public:
  AnimationFrameHeader* frame_header;
  uint8_t* frame_data;

  static AnimationFrame* try_create(uint8_t* payload, uint16_t payload_length) {
    if (payload_length < AnimationFrameHeader::header_length)
      return nullptr;

    auto header = new AnimationFrameHeader;
    header->index = payload[1] << 8 | payload[0];
    header->bytes_per_frame = payload[3] << 8 | payload[2];
    header->number_of_rows = payload[4];
    header->number_of_columns = payload[5];
    header->frame_time_ms = payload[7] << 8 | payload[6];
    header->number_of_animation_repeats = payload[8];

    if (header->bytes_per_frame > (payload_length - AnimationFrameHeader::header_length))
      return nullptr;
    return new AnimationFrame(header, payload, payload_length);
  }

  AnimationFrame(AnimationFrameHeader* header, uint8_t* payload, uint16_t payload_length) {
    frame_header = header;
    uint16_t frame_data_length = payload_length - AnimationFrameHeader::header_length;
    frame_data = new uint8_t[frame_data_length];
    for (uint16_t i = 0; i < frame_data_length; i++)
      frame_data[i] = payload[AnimationFrameHeader::header_length + i];
  }

  ~AnimationFrame() {
    delete frame_header;
    delete frame_data;
  }
};

class AnimationFramesMessage final : public Message {
public:
  Message::Type get_type() const override {
    return Message::Type::AnimationFrames;
  }

  uint16_t get_number_of_frames() const {
    return m_number_of_frames;
  }

  AnimationFrame* get_frame(uint16_t frame_index) {
    return frame_index < m_number_of_frames ? m_messages[frame_index] : nullptr;
  }

  SerialCommunicator::ReadResult read_message(HardwareSerial& serial_interface, SerialCommunicator& communicator) override {
    uint16_t frame_counter = 0;
    uint16_t crc, payload_length, calculated_crc;
  
    while (frame_counter < m_number_of_frames) {
      if (!Message::try_read_crc_and_payload_length(serial_interface, &crc, &payload_length)) {
        communicator.send_result(SerialCommunicator::ReadResult::UnexpectedData);
        return SerialCommunicator::ReadResult::UnexpectedData;
      }

      uint8_t payload_buffer[payload_length];
      if (serial_interface.readBytes(payload_buffer, payload_length) != payload_length) {
        communicator.send_result(SerialCommunicator::ReadResult::UnexpectedData);
        return SerialCommunicator::ReadResult::UnexpectedData;
      }

      calculated_crc = Message::compute_crc16_checksum(payload_buffer, payload_length);

      if (calculated_crc != crc) {
        communicator.send_result(SerialCommunicator::ReadResult::ChecksumMismatch);
        continue;
      }

      AnimationFrame* msg = AnimationFrame::try_create(payload_buffer, payload_length);
      if (msg == nullptr) {
        communicator.send_result(SerialCommunicator::ReadResult::UnexpectedData);
        return SerialCommunicator::ReadResult::UnexpectedData;
      }

      m_messages[frame_counter++] = msg;
      communicator.send_result(SerialCommunicator::ReadResult::Ok);
    }

    return SerialCommunicator::ReadResult::Ok;
  }

  static AnimationFramesMessage* create(uint16_t number_of_frames) {
    return new AnimationFramesMessage(number_of_frames);
  }

  ~AnimationFramesMessage() override {
    for (uint16_t i = 0; i < m_number_of_frames; i++)
      delete m_messages[i];
    delete m_messages;
  }
private:
  uint16_t m_number_of_frames;
  AnimationFrame** m_messages;

  AnimationFramesMessage(uint16_t number_of_frames)
    : m_number_of_frames(number_of_frames)
  {
    m_messages = new AnimationFrame*[number_of_frames];
  }
};

struct Coordinate {
  uint8_t x;
  uint8_t y;
};

class GameOfLifeConfigMessage final : public Message {
public:
  Coordinate* coordinates;
  uint16_t number_of_coordinates;
  uint16_t time_between_ticks;

  Message::Type get_type() const override {
    return Message::Type::GameOfLifeConfig;
  }

  static GameOfLifeConfigMessage* create() {
    return new GameOfLifeConfigMessage();
  }
    
  SerialCommunicator::ReadResult read_message(HardwareSerial& serial_interface, SerialCommunicator& communicator) override {
    uint16_t crc, payload_length, calculated_crc;
    while (true) {
      if (!Message::try_read_crc_and_payload_length(serial_interface, &crc, &payload_length)) {
        return SerialCommunicator::ReadResult::UnexpectedData;
      }
      uint8_t data_buffer[payload_length];
      if (serial_interface.readBytes(data_buffer, payload_length) != payload_length)
        return SerialCommunicator::ReadResult::UnexpectedData;

      calculated_crc = Message::compute_crc16_checksum(data_buffer, payload_length);
      if (calculated_crc != crc) {
        communicator.send_result(SerialCommunicator::ReadResult::ChecksumMismatch);
        delay(50);
        continue;
      }

      time_between_ticks = data_buffer[1] << 8 | data_buffer[0];
      number_of_coordinates = (payload_length - 2) / 2;
      coordinates = new Coordinate[number_of_coordinates];
      for (uint16_t i = 0; i < number_of_coordinates; i++)
        coordinates[i] = { data_buffer[2 + (i * 2)], data_buffer[2 + ((i * 2) + 1)] };

      communicator.send_result(SerialCommunicator::ReadResult::Ok);
      break;
    }

    return SerialCommunicator::ReadResult::Ok;
  }

  ~GameOfLifeConfigMessage() override {
    delete coordinates;
  }
private:
  GameOfLifeConfigMessage() { }
};