#include "TextScroller.h"
#include "TextUtilities.h"

TextScroller::TextScroller(Screen* screen)
  : m_screen(screen)
{   }

void TextScroller::init(char* text_to_scroll, TextScroller::Direction direction, uint8_t character_spacing, uint8_t y_origin) {
  m_text_to_scroll = text_to_scroll;
  m_direction = direction;
  m_character_spacing = character_spacing;
  m_y_origin = y_origin;

  m_text_number_of_columns = TextUtilities::get_number_of_columns_for_text(m_text_to_scroll, m_character_spacing);
  m_current_start_column = (m_direction == TextScroller::Direction::LEFT_TO_RIGHT) ? m_screen->screen_width() - 1 : -m_text_number_of_columns;
  m_current_end_column = (m_direction == TextScroller::Direction::LEFT_TO_RIGHT) ? m_current_start_column + m_text_number_of_columns : 0;
  m_delta = (m_direction == TextScroller::Direction::LEFT_TO_RIGHT) ? -1 : 1;

  m_is_done = false;
}

void TextScroller::tick() {
  if (is_done())
    return;

  m_screen->clear(false);
  TextUtilities::write_text_at_position(m_screen, m_text_to_scroll, m_current_start_column, m_y_origin, m_character_spacing, TextUtilities::TextMode::NOT_PERSISTENT);
  m_current_start_column += m_delta;
  m_current_end_column += m_delta;

  m_is_done = (m_direction == TextScroller::Direction::LEFT_TO_RIGHT) ? m_current_end_column < 0 : m_current_start_column == m_screen->screen_width();

  if (is_done())
    m_text_to_scroll = nullptr;
}

void TextScroller::terminate() {
  m_text_to_scroll = nullptr;
  m_is_done = true;
  m_screen->clear(true);
}

bool TextScroller::is_done() const {
  return m_is_done;
}