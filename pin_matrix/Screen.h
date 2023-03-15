#pragma once

#include <stdint.h>

class Screen {
public:
  enum class PixelValue {
    CURRENT,
    PREVIOUS,
    PERSISTENT
  };
    
  uint8_t screen_width() const;
  uint8_t screen_height() const;
  Screen(uint8_t, uint8_t);

  void update_pixel(int16_t, int16_t);
  void set_value_for_pixel(int16_t, int16_t, Screen::PixelValue);
  void unset_value_for_pixel(int16_t, int16_t, Screen::PixelValue);
  bool is_pixel_value_set(uint8_t, uint8_t, Screen::PixelValue) const;
  void clear(bool);
  
  ~Screen();
  
private:
  uint8_t *m_screen;
  uint8_t m_screen_width;
  uint8_t m_screen_height;

  static const uint8_t current_value_bit;
  static const uint8_t previous_value_bit;
  static const uint8_t persistant_value_bit;

  uint16_t get_pixel_idx(uint8_t, uint8_t) const;
  uint8_t get_bit_for_value(Screen::PixelValue) const;
  void set_or_unset_value_for_pixel(int16_t, int16_t, Screen::PixelValue, bool); 
};