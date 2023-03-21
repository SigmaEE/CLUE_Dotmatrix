#pragma once

#include <Arduino.h>
#include "SerialCommunicator.h"

class Message {
public:
  enum class Type {
    AnimationFrames,
    GameOfLifeConfig
  };

  virtual Type get_type() const = 0;

  virtual uint8_t* get_message_data() const {
    return m_message_data;
  }

  virtual uint16_t get_message_size() const {
    return m_message_size;
  }

  virtual SerialCommunicator::ReadResult read_message(HardwareSerial&, SerialCommunicator&) = 0;

  const static uint8_t animation_frames_message_identifier = 0x10;
  const static uint8_t game_of_life_config_message_identifier = 0x11;

  virtual ~Message() { 
    delete(m_message_data);
  }

protected:
  uint8_t* m_message_data;
  uint16_t m_message_size;

  const static uint16_t crc16_polynomial = 0x1021;

  static bool try_read_crc_and_payload_length(HardwareSerial& serial_interface, uint16_t* crc, uint16_t* payload_length) {
    uint8_t header_buffer[4];
    if (serial_interface.readBytes(header_buffer, 4) != 4)
      return false;

    *crc = header_buffer[1] << 8 | header_buffer[0];
    *payload_length = header_buffer[3] << 8 | header_buffer[2];
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

class AnimationFramesMessage final : public Message {
public:
  Message::Type get_type() const override {
    return Message::Type::AnimationFrames;
  }

  uint16_t get_number_of_frames() const {
    return m_number_of_frames;
  }

  uint16_t get_bytes_per_frame() const {
    return m_bytes_per_frame;
  }

  uint8_t get_number_of_rows() const {
    return m_number_of_rows;
  }

  uint8_t get_number_of_columns() const {
    return m_number_of_columns;
  }

  SerialCommunicator::ReadResult read_message(HardwareSerial& serial_interface, SerialCommunicator& communicator) override {
    uint16_t frame_counter = 0;
    uint16_t current_frame_start_idx;
    uint16_t crc, payload_length, calculated_crc;
  
    while (frame_counter < m_number_of_frames) {
      if (!Message::try_read_crc_and_payload_length(serial_interface, &crc, &payload_length)) {
        communicator.send_result(SerialCommunicator::ReadResult::UnexpectedData);
        return SerialCommunicator::ReadResult::UnexpectedData;
      }
      uint8_t frame_buffer[payload_length];
      if (serial_interface.readBytes(frame_buffer, payload_length) != payload_length) {
        communicator.send_result(SerialCommunicator::ReadResult::UnexpectedData);
        return SerialCommunicator::ReadResult::UnexpectedData;
      }

      calculated_crc = Message::compute_crc16_checksum(frame_buffer, payload_length);
      if (calculated_crc != crc) {
        communicator.send_result(SerialCommunicator::ReadResult::ChecksumMismatch);
        continue;
      }

      current_frame_start_idx = m_bytes_per_frame * frame_counter;
      for (uint16_t i = 0; i < m_bytes_per_frame; i++)
        m_message_data[current_frame_start_idx + i] = frame_buffer[i];
      communicator.send_result(SerialCommunicator::ReadResult::Ok);
      frame_counter++;
    }

    return SerialCommunicator::ReadResult::Ok;
  }

  static AnimationFramesMessage* try_create(HardwareSerial& serial_interface) {
    uint8_t header_buffer[6];
    if (serial_interface.readBytes(header_buffer, 6) == 6) {
      return new AnimationFramesMessage(header_buffer[1] << 8 | header_buffer[0],
                          header_buffer[3] << 8 | header_buffer[2],
                          header_buffer[4], header_buffer[5]);
    }
    else {
      return nullptr;
    }
  }

private:
  uint16_t m_number_of_frames;
  uint16_t m_bytes_per_frame;
  uint8_t m_number_of_rows;
  uint8_t m_number_of_columns;

  AnimationFramesMessage(uint16_t number_of_frames, uint16_t bytes_per_frame, uint8_t number_of_rows, uint8_t number_of_columns)
    : m_number_of_frames(number_of_frames),
    m_bytes_per_frame(bytes_per_frame),
    m_number_of_rows(number_of_rows),
    m_number_of_columns(number_of_columns)
  {
    m_message_size = bytes_per_frame * m_number_of_frames;
    m_message_data = new uint8_t[m_message_size]; 
  }
};

class GameOfLifeConfigMessage final : public Message {
public:
  Message::Type get_type() const override {
    return Message::Type::GameOfLifeConfig;
  }

  static GameOfLifeConfigMessage* try_create() {
    return new GameOfLifeConfigMessage();
  }
    
  SerialCommunicator::ReadResult read_message(HardwareSerial& serial_interface, SerialCommunicator& communicator) override {
    uint16_t crc, payload_length, calculated_crc;
    while (true) {
      if (!Message::try_read_crc_and_payload_length(serial_interface, &crc, &payload_length))
        return SerialCommunicator::ReadResult::UnexpectedData;

      uint8_t frame_buffer[payload_length];
      if (serial_interface.readBytes(frame_buffer, payload_length) != payload_length)
        return SerialCommunicator::ReadResult::UnexpectedData;

      calculated_crc = Message::compute_crc16_checksum(frame_buffer, payload_length);
      if (calculated_crc != crc) {
        communicator.send_result(SerialCommunicator::ReadResult::ChecksumMismatch);
        delay(50);
        continue;
      }

      m_message_size = payload_length;
      m_message_data = new uint8_t[m_message_size]; 
      for (uint16_t i = 0; i < m_message_size; i++)
        m_message_data[i] = frame_buffer[i];

      communicator.send_result(SerialCommunicator::ReadResult::Ok);
      break;
    }

    return SerialCommunicator::ReadResult::Ok;
  }

private:
  GameOfLifeConfigMessage() { }
};