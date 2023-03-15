#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include "Animator.h"
#include "DateAndTimeStringBuilder.h"
#include "GameOfLife.h"
#include "Screen.h"
#include "RevealTextAnimation.h"
#include "TextScroller.h"
#include "TextUtilities.h"

#define IC1_ENABLE 3
#define IC1_DATA 8
#define IC1_A0 2
#define IC1_A1 10
#define IC1_A2 1
#define IC1_B0 9
#define IC1_B1 0
#define IC3_ENABLE 6
#define IC2_ENABLE 11
#define IC2_IC3_A0 7
#define IC2_IC3_A1 5
#define IC2_IC3_A2 13
#define IC2_IC3_B0 12
#define IC2_IC3_B1 4

#define MATRIX_WIDTH 28
#define MATRIX_HEIGHT 16
#define CHAR_SPACING 1
#define DEFAULT_Y_ORIGIN 4

#define REFRESH_RATE_MS 5
#define ENABLE_MS 1

#define DEBUG 0

/*
 Reading the truth table (see datasheet for FP2800A) for the decoder chip
 as a five-bit number (B1 B0 A2 A1 A0), the row index is mapped as follows:

 Row Idx | Output
 -------------------
 0 - 6      | 1 - 7
 7 - 13     | 9 - 15
 14 - 20    | 17 - 23
 21 - 27    | 25 - 31

 The column mapping was by testing each possible input, i.e. it can not be deduced from the data sheet alone. */

#define MAP_ROW_IDX_TO_OUTPUT(x) (x + 1 + (x / 7))
const uint8_t COLUMN_MAP[] = { 1, 2, 3, 16, 17, 18, 19, 9, 10, 11, 24, 25, 26, 27, 5, 6, 7, 20, 21, 22, 23, 13, 14, 15, 28, 29, 30, 31 };

bool clear_at_startup = true;
TextScroller::Direction scroll_direction = TextScroller::Direction::LEFT_TO_RIGHT;
Screen screen(MATRIX_WIDTH, MATRIX_HEIGHT);
ThreeWire wire_setup(A3, A4, A2); // IO, SCLK, CE
RtcDS1302<ThreeWire> rtc(wire_setup);


void write_to_column_outputs(uint8_t v) {
  digitalWrite(IC1_A0, bitRead(v, 0) == 0 ? LOW : HIGH);
  digitalWrite(IC1_A1, bitRead(v, 1) == 0 ? LOW : HIGH);
  digitalWrite(IC1_A2, bitRead(v, 2) == 0 ? LOW : HIGH);
  digitalWrite(IC1_B0, bitRead(v, 3) == 0 ? LOW : HIGH);
  digitalWrite(IC1_B1, bitRead(v, 4) == 0 ? LOW : HIGH);
}

void write_to_row_outputs(uint8_t v) {
  digitalWrite(IC2_IC3_A0, bitRead(v, 0) == 0 ? LOW : HIGH);
  digitalWrite(IC2_IC3_A1, bitRead(v, 1) == 0 ? LOW : HIGH);
  digitalWrite(IC2_IC3_A2, bitRead(v, 2) == 0 ? LOW : HIGH);
  digitalWrite(IC2_IC3_B0, bitRead(v, 3) == 0 ? LOW : HIGH);
  digitalWrite(IC2_IC3_B1, bitRead(v, 4) == 0 ? LOW : HIGH);
}

void update_screen() {
#if DEBUG
  dump_screen_on_serial_port();
#else
  update_pin_matrix();
#endif  
}

#if DEBUG
void dump_screen_on_serial_port() {
  Serial.print("\n---------------------\n   ");
  for (uint8_t i = 0; i < screen.screen_width(); i++) {
    Serial.print(i + 1);
    Serial.print(" ");
    if (i < 9)
      Serial.print(" ");
  }
  Serial.println("");

  uint8_t pixel_is_set;
  for (uint8_t j = 0; j < screen.screen_height(); j++) {
    for (uint8_t i = 0; i < screen.screen_width(); i++) {
      if (i == 0) {
        Serial.print(j + 1);
        Serial.print(" ");
        if (j < 9)
          Serial.print(" ");
      }
      
      Serial.print(screen.is_pixel_value_set(i, j, Screen::PixelValue::CURRENT) ? "x  " : ".  ");
      screen.update_pixel(i, j);
    }

    Serial.println("");
  }
}
#else
void update_pin_matrix() {
  uint8_t chip_to_activate;
  bool bit_should_be_set;
  for (uint8_t i = 0; i < screen.screen_width(); i++) {
    for (uint8_t j = 0; j <  screen.screen_height(); j++) {
      bit_should_be_set = screen.is_pixel_value_set(i, j, Screen::PixelValue::CURRENT);
      if (bit_should_be_set ==  screen.is_pixel_value_set(i, j, Screen::PixelValue::PREVIOUS))
        continue;

      // Set the address and data pin
      write_to_column_outputs(COLUMN_MAP[i]);
      write_to_row_outputs(MAP_ROW_IDX_TO_OUTPUT(j));
      
      digitalWrite(IC1_DATA, bit_should_be_set ? LOW : HIGH);

      // Activate the outputs
      chip_to_activate = bit_should_be_set ? IC3_ENABLE : IC2_ENABLE;
      digitalWrite(IC1_ENABLE, HIGH);
      digitalWrite(chip_to_activate, HIGH);
      
      delay(ENABLE_MS);

      // Set the output low
      digitalWrite(IC1_ENABLE, LOW);
      digitalWrite(chip_to_activate, LOW);

      delay(REFRESH_RATE_MS);
      screen.update_pixel(i, j);
    }
  }
}
#endif

void setup() {
  // Set the mode for all output pins
  pinMode(IC1_ENABLE, OUTPUT);
  pinMode(IC1_DATA, OUTPUT);
  pinMode(IC1_B0, OUTPUT);
  pinMode(IC1_B1, OUTPUT);
  pinMode(IC1_A0, OUTPUT);
  pinMode(IC1_A1, OUTPUT);
  pinMode(IC1_A2, OUTPUT);
  pinMode(IC2_ENABLE, OUTPUT);
  pinMode(IC3_ENABLE, OUTPUT);
  pinMode(IC2_IC3_B0, OUTPUT);
  pinMode(IC2_IC3_B1, OUTPUT);
  pinMode(IC2_IC3_A0, OUTPUT);
  pinMode(IC2_IC3_A1, OUTPUT);
  pinMode(IC2_IC3_A2, OUTPUT);

  // Ensure all enable pins are initially set low
  digitalWrite(IC1_ENABLE, LOW);
  digitalWrite(IC2_ENABLE, LOW);
  digitalWrite(IC3_ENABLE, LOW);

  // Prime the matrix som that the update_screen method will try to reset all pixels at startup
  if (clear_at_startup) {
    for (uint8_t i = 0; i < screen.screen_width(); i++) {
      for (uint8_t j = 0; j < screen.screen_height(); j++) {
        // Setting the previous value bit ensures that all dots are set to zero at the first update_screen call
        screen.set_value_for_pixel(i, j, Screen::PixelValue::PREVIOUS);
      }
    }
  }

  // Setup the real-time clock
  rtc.Begin();
  RtcDateTime program_compiled_timestamp = RtcDateTime(__DATE__, __TIME__);

  if (!rtc.IsDateTimeValid()) 
    rtc.SetDateTime(program_compiled_timestamp);

  if (rtc.GetIsWriteProtected())
      rtc.SetIsWriteProtected(false);

  if (!rtc.GetIsRunning())
      rtc.SetIsRunning(true);
  
  RtcDateTime now = rtc.GetDateTime();
  if (now < program_compiled_timestamp) 
    rtc.SetDateTime(program_compiled_timestamp);
  else if ((now.TotalSeconds() - program_compiled_timestamp.TotalSeconds()) > 10)
    rtc.SetDateTime(program_compiled_timestamp);

  if (!rtc.GetIsWriteProtected()) 
    rtc.SetIsWriteProtected(true);

#if DEBUG
  Serial.begin(9600);
#endif

  screen.clear(true);
  update_screen();
}

void scroll_text(char* text) {
  TextScroller scroller(&screen, text, scroll_direction, CHAR_SPACING, DEFAULT_Y_ORIGIN);
  while (!scroller.is_done()) {
    scroller.tick_animation();
    update_screen();
  }
}

void reveal_text(char* text, uint8_t number_of_simultaneous_rows_or_columns, RevealTextAnimation::Mode mode) {
  RevealTextAnimation text_revealer(&screen, text, CHAR_SPACING, number_of_simultaneous_rows_or_columns, DEFAULT_Y_ORIGIN, mode);
  while (!text_revealer.is_done()) {
    text_revealer.tick_animation();
    update_screen();
  }
}

void loop() {
  static bool first = true;
  static bool draw_colon_separator = true; 
  static uint8_t last_minute, last_second, last_hour, x_origin;
  static char time_string[8];
  static char date_string[30]; // Longest possible date should be a Wednesday in September
  static GameOfLife game_of_life(&screen, GameOfLife::starting_configuration::SEED_1);
  static Animator animator(&screen, Animator::Animation::LETTERS_TEST, 1);

  while (!animator.is_done() && first) {
    animator.tick_animation();
    update_screen();
  }
  RtcDateTime now = rtc.GetDateTime();

  // game_of_life.init();
  // while(true) {
  //   game_of_life.tick();
  //   update_screen();
  //   delay(100);
  // }

  if (!now.IsValid())
    scroll_text("COULD NOT GET TIME - RESTART PLEASE");

  if (last_hour != now.Hour() && !first) {
    DateAndTimeStringBuilder::build_date_string(date_string, now.Year(), now.Month(), now.Day(), now.DayOfWeek());
    scroll_text(date_string);
    now = rtc.GetDateTime();
  }

  if (first || now.Minute() != last_minute || now.Second() != last_second) {
    screen.clear(true);
    DateAndTimeStringBuilder::build_time_string(time_string, now.Hour(), now.Minute(), draw_colon_separator);
    if (first) {
      reveal_text(time_string, 2, RevealTextAnimation::Mode::Columnwise);
      first = false;
    }
    else {
      x_origin = TextUtilities::get_x_origin_of_centered_text(&screen, time_string, CHAR_SPACING);
      TextUtilities::write_text_at_position(&screen, time_string, x_origin, DEFAULT_Y_ORIGIN, CHAR_SPACING, TextUtilities::TextMode::NOT_PERSISTENT);
    }

    update_screen();

    last_minute = now.Minute();
    if (now.Second() != last_second)
      draw_colon_separator = !draw_colon_separator;
    last_second = now.Second();
    last_hour = now.Hour();
  }

  delay(100);
}
