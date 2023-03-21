#pragma once

#include "Animator.h"

Animator::Animator(Screen* screen)
  : m_screen(screen)
{ }

void Animator::init(AnimationFramesMessage* animation) {
  m_is_done = false;
  m_animation_counter = 0;
  m_frame_counter = 0;

  m_number_of_repeats = 1;  // TODO: Include in message
  m_number_of_frames = animation->get_number_of_frames();
  m_number_of_rows = animation->get_number_of_rows();
  m_number_of_columns = animation->get_number_of_columns();
  m_number_of_bytes_per_frame = animation->get_bytes_per_frame();
  m_animation = animation;

  uint8_t number_of_columns = m_number_of_columns;
  m_number_of_bytes_per_row = 0;
  while (number_of_columns != 0) {
    m_number_of_bytes_per_row++;
    number_of_columns >>= 1;
  }

  m_number_of_bytes_per_row--;
}

void Animator::tick_animation() {
  if (is_done())
    return;

  uint16_t idx = (uint16_t)m_frame_counter * m_number_of_bytes_per_frame;
  uint8_t bit_shift = 7;
  uint8_t value;
  int16_t x;

  for (uint8_t i = 0; i < m_number_of_rows; i++) {
    for (uint8_t j = 0; j < m_number_of_bytes_per_row; j++) {
      value = m_animation->get_message_data()[idx];
      while (true) {
        x = (j << 3) + (7 - bit_shift);
        if (x < m_number_of_columns) {
          if (((value >> bit_shift) & 0x01) == 0x01)
            m_screen->set_value_for_pixel(x, (int16_t)i, Screen::PixelValue::CURRENT);
          else
            m_screen->unset_value_for_pixel(x, (int16_t)i, Screen::PixelValue::CURRENT);
        }

        if (bit_shift == 0)
          break;

        bit_shift--;
      }

      bit_shift = 7;
      idx++;
    }
  }

  m_frame_counter++;
  if (m_frame_counter == m_number_of_frames) {
    m_frame_counter = 0;
    m_animation_counter++;
  }

  if (m_animation_counter == m_number_of_repeats) {
    m_animation = nullptr;
    m_is_done = true;
  }
}

bool Animator::is_done() const {
  return m_is_done;
}