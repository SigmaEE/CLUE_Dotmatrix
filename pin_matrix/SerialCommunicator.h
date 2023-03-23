#pragma once

#include <Arduino.h>

class Message;

class SerialCommunicator {

public:
  enum class ReadResult {
    Ok = 0x01,
    InternalError = 0x02,
    ChecksumMismatch = 0x03,
    UnexpectedData = 0x05
  };

  SerialCommunicator(HardwareSerial&);
  bool try_read_header();
  SerialCommunicator::ReadResult read_message();
  void send_result(SerialCommunicator::ReadResult);
  Message* get_current_message() const;
  void flush_message();

private:
  HardwareSerial& m_serial_interface;
  Message* m_current_message;

  const static uint8_t transmission_header_number_of_bytes;
};