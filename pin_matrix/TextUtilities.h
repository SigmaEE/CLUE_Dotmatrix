#pragma once

#include <stdint.h>
#include "Screen.h"

class TextUtilities {
public:
  enum class TextMode {
    PERSISTENT_VISIBLE,
    PERSISTENT_INVISIBLE,
    NOT_PERSISTENT
  };

  uint16_t static get_number_of_columns_for_text(char*, uint8_t);
  void static write_text_at_position(Screen*, char*, int16_t, int16_t, uint8_t, TextUtilities::TextMode);
  uint8_t static get_x_origin_of_centered_text(Screen*, char*, uint8_t);

private:
  TextUtilities();

  static uint8_t write_character_at_position(Screen*, char, int16_t, int16_t, TextUtilities::TextMode);
};