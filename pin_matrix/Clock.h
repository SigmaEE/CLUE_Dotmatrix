#pragma once

#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#include "Screen.h"
#include "DisplayMode.h"

class Clock {
public:
  bool run_seconds_animation;
  bool run_reveal_text_animation;

  Clock(Screen*, uint8_t, uint8_t);
  
  void init(RtcDS1302<ThreeWire>*, RtcDateTime);

  void tick();

  bool wants_mode_change() const;

  DisplayMode new_mode() const;

  char* string_to_scroll() const;

  char* string_to_reveal() const;

private:
  Screen* m_screen;
  RtcDS1302<ThreeWire>* m_rtc;
  uint8_t m_char_spacing;
  uint8_t m_y_origin;
  char m_time_string[8];
  char m_date_string[30]; // Longest possible date should be a Wednesday in September
  bool m_wants_mode_change;
  bool m_draw_colon_separator;
  bool m_rtc_error;
  DisplayMode m_new_mode;
  uint8_t m_last_minute;
  uint8_t m_last_second;
  uint8_t m_last_hour;
  uint8_t m_x_origin;
};