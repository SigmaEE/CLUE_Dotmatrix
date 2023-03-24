#pragma once

#include <stdint.h>

class DateAndTimeStringBuilder {
public:
  static void get_day_of_week(char*, bool, uint8_t);
  static void get_name_of_month(char*, bool, uint8_t);
  static void build_time_string(char*, uint8_t, uint8_t, uint8_t, bool, bool);
  static void build_date_string(char*, bool, uint16_t, uint8_t, uint8_t, uint8_t);

private:
  const static uint8_t offset_to_ascii_zero;
  DateAndTimeStringBuilder();

  static char get_ones_digit(uint8_t);
  static char get_ones_digit(uint16_t);
  static char get_tens_digit(uint8_t);
  static char get_tens_digit(uint16_t);
  static char get_hundreds_digit(uint16_t);
  static char get_thousands_digit(uint16_t);
};