#include "GameOfLife.h"

GameOfLife::GameOfLife(Screen* screen) 
  : m_screen(screen)
  { }

void GameOfLife::init(GameOfLifeConfigMessage* config) {
  m_screen->clear(true);

  uint16_t number_of_coordinates = config->get_message_size() / 2;
  uint8_t x, y;
  for (uint16_t i = 0; i < number_of_coordinates; i++) {
    x = config->get_message_data()[i * 2];
    y = config->get_message_data()[(i * 2) + 1];
    m_screen->set_value_for_pixel(x, y, Screen::PixelValue::CURRENT);
  }
}

void GameOfLife::tick() {
  bool cell_is_alive;
  uint8_t number_of_alive_neighbors, x_to_check, y_to_check;
  
  for (int8_t i = 0; i < m_screen->screen_width(); i++) {
    for (int8_t j = 0; j < m_screen->screen_height(); j++) {
      cell_is_alive = m_screen->is_pixel_value_set((uint8_t)i, (uint8_t)j, Screen::PixelValue::PREVIOUS);

      number_of_alive_neighbors = 0;
      for (int8_t r = -1; r < 2; r++) {
        for (int8_t c = -1; c < 2; c++) {
          if (r == 0 && c == 0)
            continue;

          x_to_check = (i + r < 0) ? m_screen->screen_width() - 1 : ((uint8_t)(i + r) == m_screen->screen_width() ? 0 : (uint8_t)(i + r));
          y_to_check = (j + c < 0) ? m_screen->screen_height() - 1 : ((uint8_t)(j + c) == m_screen->screen_height() ? 0 : (uint8_t)(j + c));
          
          if (m_screen->is_pixel_value_set(x_to_check, y_to_check, Screen::PixelValue::PREVIOUS))
            number_of_alive_neighbors++;
        }
      }

      // Any live cell with two or three live neighbors survives. All other live cells die in the next generation.
      if (cell_is_alive && (number_of_alive_neighbors < 2 || number_of_alive_neighbors > 3))
        m_screen->unset_value_for_pixel(i, j, Screen::PixelValue::CURRENT);
      // Any dead cell with three live neighbours becomes a live cell.
      else if (!cell_is_alive && number_of_alive_neighbors == 3)
        m_screen->set_value_for_pixel(i, j, Screen::PixelValue::CURRENT); 
    }
  }
}

bool GameOfLife::is_done() const {
  bool has_change, last_value, current_value;
  has_change = false;
  for (uint8_t i = 0; i < m_screen->screen_width(); i++) {
    for (uint8_t j = 0; j < m_screen->screen_height(); j++) {
      last_value = m_screen->is_pixel_value_set(i, j, Screen::PixelValue::PREVIOUS);
      current_value = m_screen->is_pixel_value_set(i, j, Screen::PixelValue::CURRENT);
      if (last_value != current_value) {
        has_change = true;
        break;
      }
    }
  }

  return !has_change;
}