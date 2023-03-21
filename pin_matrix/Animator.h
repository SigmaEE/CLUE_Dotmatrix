#pragma once

#include <stdint.h>
#include "Screen.h"
#include "Messages.h"

class Animator {
public:
  Animator(Screen*);

  void init(AnimationFramesMessage* animation);

  void tick_animation();

  bool is_done() const;

private:
  Screen* m_screen;
  AnimationFramesMessage* m_animation;

  bool m_is_done;
  uint8_t m_number_of_repeats;
  uint8_t m_animation_counter;
  uint8_t m_frame_counter;
  
  uint8_t m_number_of_frames;
  uint8_t m_number_of_rows;
  uint8_t m_number_of_columns;
  
  uint8_t m_number_of_bytes_per_row;
  uint8_t m_number_of_bytes_per_frame;
};