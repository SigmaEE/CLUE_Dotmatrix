#include "GameOfLife.h"

GameOfLife::GameOfLife(Screen* screen) 
  : m_screen(screen)
  { }

void GameOfLife::init(GameOfLifeConfigMessage* config) {
  m_screen->clear(true);

  for (uint16_t i = 0; i < config->number_of_coordinates; i++)
    m_screen->set_value_for_pixel(config->coordinates[i].x, config->coordinates[i].y, Screen::PixelValue::CURRENT);

  m_is_done = false;
  m_time_between_ticks = config->time_between_ticks;
  m_timestamp_at_last_tick = 0;
}

void GameOfLife::tick() {
  if (is_done())
    return;

  if (m_timestamp_at_last_tick == 0)
    m_timestamp_at_last_tick = millis();

  uint32_t current_timestamp = millis();
  if (current_timestamp - m_timestamp_at_last_tick < m_time_between_ticks)
    return;

  m_timestamp_at_last_tick = current_timestamp;
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

  m_is_done = check_is_done();
}

void GameOfLife::terminate() {
  m_is_done = true;
  m_screen->clear(true);
}

bool GameOfLife::is_done() const {
  return m_is_done;
}

bool GameOfLife::check_is_done() const {
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