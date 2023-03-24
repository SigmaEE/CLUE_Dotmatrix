#pragma once

#include <stdint.h>

class DotMatrixFont {
public:
  static uint8_t get_width_of_character(char);
  static uint8_t get_height_of_character();
  static uint8_t* get_character(char);
  static uint8_t to_character_matrix_idx(uint8_t, uint8_t);

private:
  DotMatrixFont();
  const static uint8_t character_height;
  const static uint8_t CHAR_A[];
  const static uint8_t CHAR_B[];
  const static uint8_t CHAR_C[];
  const static uint8_t CHAR_D[];
  const static uint8_t CHAR_E[];
  const static uint8_t CHAR_F[];
  const static uint8_t CHAR_G[];
  const static uint8_t CHAR_H[];
  const static uint8_t CHAR_I[];
  const static uint8_t CHAR_J[];
  const static uint8_t CHAR_K[];
  const static uint8_t CHAR_L[];
  const static uint8_t CHAR_M[];
  const static uint8_t CHAR_N[];
  const static uint8_t CHAR_O[];
  const static uint8_t CHAR_P[];
  const static uint8_t CHAR_Q[];
  const static uint8_t CHAR_R[];
  const static uint8_t CHAR_S[];
  const static uint8_t CHAR_T[];
  const static uint8_t CHAR_U[];
  const static uint8_t CHAR_V[];
  const static uint8_t CHAR_W[];
  const static uint8_t CHAR_X[];
  const static uint8_t CHAR_Y[];
  const static uint8_t CHAR_Z[];
  const static uint8_t CHAR_0[];
  const static uint8_t CHAR_1[];
  const static uint8_t CHAR_2[];
  const static uint8_t CHAR_3[];
  const static uint8_t CHAR_4[];
  const static uint8_t CHAR_5[];
  const static uint8_t CHAR_6[];
  const static uint8_t CHAR_7[];
  const static uint8_t CHAR_8[];
  const static uint8_t CHAR_9[];
  const static uint8_t CHAR_COLON[];
  const static uint8_t CHAR_SPACE[];
  const static uint8_t CHAR_HYPHEN[];
  const static uint8_t CHAR_DOT[];
};