#include "RevealTextAnimation.h"
#include "TextUtilities.h"

RevealTextAnimation::RevealTextAnimation(Screen* screen)
: m_screen(screen)
{ }

void RevealTextAnimation::init(char* text_to_reveal, uint8_t character_spacing, uint8_t number_of_simultaneous_rows_or_columns, uint8_t y_origin, RevealTextAnimation::Mode mode) {
  m_mode = mode;
  m_number_of_simultaneous_rows_or_columns = number_of_simultaneous_rows_or_columns > 5 ? 5 : number_of_simultaneous_rows_or_columns;

  uint8_t x_origin_of_text = TextUtilities::get_x_origin_of_centered_text(m_screen, text_to_reveal, character_spacing);
  TextUtilities::write_text_at_position(m_screen, text_to_reveal, x_origin_of_text, y_origin, character_spacing, TextUtilities::TextMode::PERSISTENT_INVISIBLE);

  m_current_column = 0;
  m_current_row = 0;
  m_is_done = false;

  m_number_of_cleanup_steps_left = m_number_of_simultaneous_rows_or_columns;
}

void RevealTextAnimation::tick() {
  if (is_done())
    return;

  switch (m_mode) {
    case RevealTextAnimation::Mode::Rowwise:
    tick_falling_row_animation();
    break;

  case RevealTextAnimation::Mode::Columnwise:
    tick_falling_column_animation();
    break;
  }
}

bool RevealTextAnimation::is_done() const {
  return m_is_done;
}

void RevealTextAnimation::terminate() {
  m_is_done = true;
  m_screen->clear(true);
}

void RevealTextAnimation::tick_falling_column_animation() {
  if (m_current_column < m_screen->screen_width()) {
    for (uint8_t j = 0; j < m_screen->screen_height(); j++) {
      m_screen->set_value_for_pixel(m_current_column, j, Screen::PixelValue::CURRENT);
      if (m_current_column > (m_number_of_simultaneous_rows_or_columns - 1))
        m_screen->unset_value_for_pixel(m_current_column - m_number_of_simultaneous_rows_or_columns, j, Screen::PixelValue::CURRENT);
    }
    m_current_column++;
  }
  else if (m_number_of_cleanup_steps_left > 0) {
    for (uint8_t j = 0; j < m_screen->screen_height(); j++)
      m_screen->unset_value_for_pixel(m_screen->screen_width() - m_number_of_cleanup_steps_left, j, Screen::PixelValue::CURRENT);
    m_number_of_cleanup_steps_left--;
  }
  else {
    m_is_done = true;
  }
}

void RevealTextAnimation::tick_falling_row_animation() {
  if (m_current_row < m_screen->screen_height()) {
    for (uint8_t i = 0; i < m_screen->screen_width(); i++) {
      m_screen->set_value_for_pixel(i, m_current_row, Screen::PixelValue::CURRENT);
      if (m_current_row > (m_number_of_simultaneous_rows_or_columns - 1))
        m_screen->unset_value_for_pixel(i, m_current_row - m_number_of_simultaneous_rows_or_columns, Screen::PixelValue::CURRENT);
    }
    m_current_row++;
  }
  else if (m_number_of_cleanup_steps_left > 0) {
    for (uint8_t i = 0; i < m_screen->screen_width(); i++)
      m_screen->unset_value_for_pixel(i, m_screen->screen_height() - m_number_of_cleanup_steps_left, Screen::PixelValue::CURRENT);
    m_number_of_cleanup_steps_left--;
  }
  else {
    m_is_done = true;
  }
}

void reveal_string_falling_column_animation(char* str, uint8_t y_origin, uint8_t number_of_simultaneous_columns) {
  
}
