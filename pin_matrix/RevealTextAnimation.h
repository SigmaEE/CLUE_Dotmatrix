#pragma once

#include "Screen.h"
class RevealTextAnimation {
public:
  enum class Mode {
    Rowwise,
    Columnwise
  };

  RevealTextAnimation(Screen*);

  void init(char*, uint8_t, uint8_t, uint8_t, bool, RevealTextAnimation::Mode);

  void tick();

  void terminate();

  bool is_done() const;

private:
  Screen* m_screen;
  uint8_t m_number_of_simultaneous_rows_or_columns;
  RevealTextAnimation::Mode m_mode;
  uint8_t m_current_column;
  uint8_t m_current_row;
  uint8_t m_number_of_cleanup_steps_left;
  bool m_is_done;
  
  void tick_falling_column_animation();
  void tick_falling_row_animation();
};