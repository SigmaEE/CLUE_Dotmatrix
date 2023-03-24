#include "DateAndTimeStringBuilder.h"

void DateAndTimeStringBuilder::build_time_string(char* buffer, uint8_t hour, uint8_t minute, bool include_colon) {
  buffer[0] = get_tens_digit(hour);
  buffer[1] = get_ones_digit(hour);
  buffer[2] = ' ';
  buffer[3] = include_colon ? ':'  : ' ';
  buffer[4] = ' ';
  buffer[5] = get_tens_digit(minute);
  buffer[6] = get_ones_digit(minute);
  buffer[7] = '\0';
}

void DateAndTimeStringBuilder::get_day_of_week(char* buffer, bool get_long_format, uint8_t day_of_the_week) {
  uint8_t idx = 0;
  switch (day_of_the_week) {
    case 0:
      buffer[idx++] = 'S'; buffer[idx++] = 'U';  buffer[idx++] = 'N';
      break;
    case 1:
      buffer[idx++] = 'M'; buffer[idx++] = 'O';  buffer[idx++] = 'N';
      break;
    case 2:
      buffer[idx++] = 'T'; buffer[idx++] = 'U';  buffer[idx++] = 'E'; 
      if (get_long_format)
        buffer[idx++] = 'S';
      break;
    case 3:
      buffer[idx++] = 'W'; buffer[idx++] = 'E';  buffer[idx++] = 'D'; 
      if (get_long_format) {
        buffer[idx++] = 'N';
        buffer[idx++] = 'E';
        buffer[idx++] = 'S';
      }
      break;
    case 4:
      buffer[idx++] = 'T'; buffer[idx++] = 'H'; buffer[idx++] = 'U'; 
      if (get_long_format) {
        buffer[idx++] = 'R';
        buffer[idx++] = 'S';
      }
      break;
    case 5:
      buffer[idx++] = 'F'; buffer[idx++] = 'R';  buffer[idx++] = 'I';
      break;
    case 6:
      buffer[idx++] = 'S'; buffer[idx++] = 'A';  buffer[idx++] = 'T'; 
      if (get_long_format) {
        buffer[idx++] = 'U';
        buffer[idx++] = 'R';
      }
      break;
  }

  if (get_long_format) {
    buffer[idx++] = 'D';
    buffer[idx++] = 'A';
    buffer[idx++] = 'Y';
  }
  else {
    buffer[idx++] = '.';
  }
  buffer[idx] = '\0';
}

void DateAndTimeStringBuilder::get_name_of_month(char* buffer, bool get_long_format, uint8_t month) {
  uint8_t idx = 0;
  switch (month) {
    case 1:
      buffer[idx++] = 'J'; buffer[idx++] = 'A'; buffer[idx++] = 'N';
      if (get_long_format) {
        buffer[idx++] = 'U';
        buffer[idx++] = 'A';
        buffer[idx++] = 'R';
        buffer[idx++] = 'Y';
      }
      break;
    case 2:
      buffer[idx++] = 'F'; buffer[idx++] = 'E'; buffer[idx++] = 'B';
      if (get_long_format) {
        buffer[idx++] = 'R';
        buffer[idx++] = 'U';
        buffer[idx++] = 'A';
        buffer[idx++] = 'R';
        buffer[idx++] = 'Y';
      }
      break;
    case 3:
      buffer[idx++] = 'M'; buffer[idx++] = 'A'; buffer[idx++] = 'R';
      if (get_long_format) {
        buffer[idx++] = 'C';
        buffer[idx++] = 'H';
      }
      break;
    case 4:
      buffer[idx++] = 'A'; buffer[idx++] = 'P'; buffer[idx++] = 'R';
      if (get_long_format) {
        buffer[idx++] = 'I';
        buffer[idx++] = 'L';
      }
      break;
    case 5:
      buffer[idx++] = 'M'; buffer[idx++] = 'A'; buffer[idx++] = 'Y';
      break;
    case 6:
      buffer[idx++] = 'J'; buffer[idx++] = 'U'; buffer[idx++] = 'N';
      if (get_long_format)
        buffer[idx++] = 'E';
      break;
    case 7:
      buffer[idx++] = 'J'; buffer[idx++] = 'U'; buffer[idx++] = 'L';
      if (get_long_format)
        buffer[idx++] = 'Y';
      break;
    case 8:
      buffer[idx++] = 'A'; buffer[idx++] = 'U'; buffer[idx++] = 'G';
      if (get_long_format) {
        buffer[idx++] = 'U';
        buffer[idx++] = 'S';
        buffer[idx++] = 'T';
      }
      break;
    case 9:
      buffer[idx++] = 'S'; buffer[idx++] = 'E'; buffer[idx++] = 'P';
      if (get_long_format) {
        buffer[idx++] = 'T';
        buffer[idx++] = 'E';
        buffer[idx++] = 'M';
        buffer[idx++] = 'B';
        buffer[idx++] = 'E';
        buffer[idx++] = 'R';
      }
      break;
    case 10:
      buffer[idx++] = 'O'; buffer[idx++] = 'C'; buffer[idx++] = 'T';
      if (get_long_format) {
        buffer[idx++] = 'O';
        buffer[idx++] = 'B';
        buffer[idx++] = 'E'; 
        buffer[idx++] = 'R';
      }
      break;
    case 11:
      buffer[idx++] = 'N'; buffer[idx++] = 'O'; buffer[idx++] = 'V';
      if (get_long_format) {
        buffer[idx++] = 'E';
        buffer[idx++] = 'M';
        buffer[idx++] = 'B'; 
        buffer[idx++] = 'E';
        buffer[idx++] = 'R';
      }
      break;
    case 12:
      buffer[idx++] = 'D'; buffer[idx++] = 'E'; buffer[idx++] = 'C';
      if (get_long_format) {
        buffer[idx++] = 'E';
        buffer[idx++] = 'M';
        buffer[idx++] = 'B'; 
        buffer[idx++] = 'E';
        buffer[idx++] = 'R';
      }
      break;
  }

  if (!get_long_format && month != 5)
    buffer[idx++] = '.';
  buffer[idx] = '\0';
}

void DateAndTimeStringBuilder::build_date_string(char* buffer, bool get_long_format, uint16_t year, uint8_t month, uint8_t day_of_the_month, uint8_t day_of_the_week) {
  get_day_of_week(buffer, get_long_format, day_of_the_week);
  uint8_t idx = 0;
  while (buffer[idx] != '\0')
    idx++;

  buffer[idx++] = ' ';

  buffer[idx++] = get_tens_digit(day_of_the_month);
  buffer[idx++] = get_ones_digit(day_of_the_month);

  buffer[idx++] = ' ';

  char month_name[10];
  get_name_of_month(month_name, get_long_format, month);
  uint8_t i = 0;
  while (month_name[i] != '\0')
    buffer[idx++] = month_name[i++];

  buffer[idx++] = ' ';

  buffer[idx++] = get_thousands_digit(year);
  buffer[idx++] = get_hundreds_digit(year);
  buffer[idx++] = get_tens_digit(year);
  buffer[idx++] = get_ones_digit(year);

  buffer[idx] = '\0';
}

// '0'-'9' occupies 48 - 57 in the ASCII table
const uint8_t DateAndTimeStringBuilder::offset_to_ascii_zero = 48;

static char DateAndTimeStringBuilder::get_ones_digit(uint8_t v) { return (char)((v % 10) + offset_to_ascii_zero); }
static char DateAndTimeStringBuilder::get_ones_digit(uint16_t v) { return (char)((v % 10) + offset_to_ascii_zero); }
static char DateAndTimeStringBuilder::get_tens_digit(uint8_t v) { return (char)((v / 10) + offset_to_ascii_zero); }
static char DateAndTimeStringBuilder::get_tens_digit(uint16_t v) { return (char)(((v % 100) / 10) + offset_to_ascii_zero); }
static char DateAndTimeStringBuilder::get_hundreds_digit(uint16_t v) { return (char)(((v % 1000) / 100) + offset_to_ascii_zero); }
static char DateAndTimeStringBuilder::get_thousands_digit(uint16_t v) { return (char)((v / 1000) + offset_to_ascii_zero); }