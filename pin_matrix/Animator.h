#pragma once

#include <stdint.h>
#include "Screen.h"

class Animator {
public:
  enum class Animation {
    LETTERS_TEST
  };

  Animator(Screen*, Animator::Animation, uint8_t);

  void tick_animation();

  bool is_done() const;

private:
  Screen* m_screen;
  uint8_t* m_animation;
  bool m_is_done;
  uint8_t m_number_of_repeats;
  uint8_t m_animation_counter;
  uint8_t m_frame_counter;
  
  uint8_t m_number_of_frames;
  uint8_t m_number_of_rows;
  uint8_t m_number_of_columns;
  
  uint8_t m_number_of_bytes_per_row;
  uint8_t m_number_of_bytes_per_frame;

  const static uint8_t frame_data_header_length;
  const static uint8_t LETTERS_TEST_ANIMATION[];
};