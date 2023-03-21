#include "SerialCommunicator.h"
#include "Messages.h"

SerialCommunicator::SerialCommunicator(HardwareSerial& serial_interface)
  :  m_serial_interface(serial_interface)
  {
    m_current_message = nullptr;
  }

bool SerialCommunicator::try_read_header() {
  if (m_serial_interface.available() > 0) {
    uint8_t identifier = m_serial_interface.read();
    if (identifier == Message::animation_frames_message_identifier)
      m_current_message = AnimationFramesMessage::try_create(m_serial_interface);
    else if (identifier == Message::game_of_life_config_message_identifier)
      m_current_message = GameOfLifeConfigMessage::try_create();
  }
    
  send_result(m_current_message != nullptr ? SerialCommunicator::ReadResult::Ok : SerialCommunicator::ReadResult::UnexpectedData);
  return m_current_message != nullptr;
}

void SerialCommunicator::send_result(SerialCommunicator::ReadResult result) {
  m_serial_interface.write((uint8_t)result);
}

SerialCommunicator::ReadResult SerialCommunicator::read_message() {
  if (m_current_message == nullptr)
    return SerialCommunicator::ReadResult::InternalError;
  return m_current_message->read_message(m_serial_interface, *this);
}

Message* SerialCommunicator::get_current_message() const {
  return m_current_message;
}

void SerialCommunicator::flush_message() {
  delete(m_current_message);
  m_current_message = nullptr;
}