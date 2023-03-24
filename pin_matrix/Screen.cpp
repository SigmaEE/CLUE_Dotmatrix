#include "Screen.h"

Screen::Screen(uint8_t screen_width, uint8_t screen_height) :
  m_screen_width(screen_width),
  m_screen_height(screen_height) {
  
  m_screen = new uint8_t[screen_width * screen_height];
}

uint8_t Screen::screen_width() const { return m_screen_width; }

uint8_t Screen::screen_height() const { return m_screen_height; }

void Screen::update_pixel(int16_t x, int16_t y) {
  if (x < 0 || (x > (m_screen_width - 1)))
    return;

  if (y < 0 || (y > (m_screen_height - 1)))
    return;

  bool is_currently_set = is_pixel_value_set((uint8_t)x, (uint8_t)y, Screen::PixelValue::CURRENT);
  set_or_unset_value_for_pixel(x, y, Screen::PixelValue::PREVIOUS, is_currently_set);
}

void Screen::set_value_for_pixel(int16_t x, int16_t y, Screen::PixelValue value) { set_or_unset_value_for_pixel(x, y, value, true); }

void Screen::unset_value_for_pixel(int16_t x, int16_t y, Screen::PixelValue value) { set_or_unset_value_for_pixel(x, y, value, false); }

bool Screen::is_pixel_value_set(uint8_t x, uint8_t y, Screen::PixelValue value) const {
  return ((m_screen[get_pixel_idx(x, y)] >> get_bit_for_value(value)) & 0x01) == 1 ? true : false;
}

void Screen::clear(bool also_clear_persistent_flag) {
  clear_bounding_box({0, m_screen_width, 0, m_screen_height}, also_clear_persistent_flag);
}

void Screen::clear_bounding_box(const BoundingBox& bb, bool also_clear_persistent_flag) {
  for (uint8_t i = bb.left; i < bb.right; i++) {
    for (uint8_t j = bb.top; j < bb.bottom; j++) {
      if (also_clear_persistent_flag)
        unset_value_for_pixel(i, j, Screen::PixelValue::PERSISTENT);    
      unset_value_for_pixel(i, j, Screen::PixelValue::CURRENT);  
    }
  }
}

void Screen::set_or_unset_value_for_pixel(int16_t x, int16_t y, Screen::PixelValue value, bool set_bit) {
  if (x < 0 || (x > (m_screen_width - 1)))
    return;

  if (y < 0 || (y > (m_screen_height - 1)))
    return;

  if (set_bit) {
    m_screen[get_pixel_idx(x, y)] |= (0x01 << get_bit_for_value(value));
  }
  else {
    bool skip_clearing = value == Screen::PixelValue::CURRENT && is_pixel_value_set(x, y, Screen::PixelValue::PERSISTENT);
    if (!skip_clearing)
      m_screen[get_pixel_idx(x, y)] &= ~(0x01 << get_bit_for_value(value));
  }
}

uint16_t Screen::get_pixel_idx(uint8_t x, uint8_t y) const { 
  return (x * m_screen_height) + y;
}

uint8_t Screen::get_bit_for_value(Screen::PixelValue value) const { 
  switch (value) {
    case Screen::PixelValue::CURRENT:
      return current_value_bit;
    case Screen::PixelValue::PREVIOUS:
      return previous_value_bit;
    case Screen::PixelValue::PERSISTENT:
      return persistant_value_bit;
  }
}

Screen::~Screen() { delete m_screen; }

const uint8_t Screen::current_value_bit = 0;
const uint8_t Screen::previous_value_bit = 1;
const uint8_t Screen::persistant_value_bit = 2;
  
  