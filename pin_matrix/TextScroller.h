#pragma once

#include <stdint.h>
#include "Screen.h"

class TextScroller {
public:
  enum class Direction {
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT
  };

  TextScroller(Screen*);

  void init(char*, TextScroller::Direction, uint8_t, uint8_t);

  void tick();

  void terminate();

  bool is_done() const;

private:
  Screen* m_screen;
  uint16_t m_text_number_of_columns;
  char* m_text_to_scroll;
  TextScroller::Direction m_direction;
  uint8_t m_character_spacing;
  uint8_t m_y_origin;
  int16_t m_current_start_column;
  int16_t m_current_end_column;
  int8_t m_delta;
  bool m_is_done;
};