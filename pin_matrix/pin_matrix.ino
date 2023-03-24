#include <ThreeWire.h>  
#include <RtcDS1302.h>

#include "Animator.h"
#include "Clock.h"
#include "DisplayMode.h"
#include "GameOfLife.h"
#include "Messages.h"
#include "RevealTextAnimation.h"
#include "Screen.h"
#include "SerialCommunicator.h"
#include "TextScroller.h"
#include "DotMatrixFont.h"

// Arduino Mega pinout
#define IC1_ENABLE 22
#define IC1_B1 23
#define IC1_A2 24
#define IC1_A0 25
#define IC3_ENABLE 26
#define IC2_IC3_B1 27
#define IC2_IC3_A1 28
#define IC2_IC3_A0 29
#define IC1_DATA 30
#define IC1_B0 31
#define IC1_A1 32
#define IC2_ENABLE 33
#define IC2_IC3_B0 34
#define IC2_IC3_A2 35

#define RTS_DATA_PIN 52
#define RTS_CLOCK_PIN 51
#define RTS_RESET_PIN 53

#define MATRIX_WIDTH 28
#define MATRIX_HEIGHT 16
#define CHAR_SPACING 1

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
const uint8_t COLUMN_MAP[] = { 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 25, 26, 27, 28, 29, 30, 31 };

bool clear_at_startup = true;
TextScroller::Direction scroll_direction = TextScroller::Direction::LEFT_TO_RIGHT;
RevealTextAnimation::Mode reveal_mode = RevealTextAnimation::Mode::Columnwise;
uint8_t number_of_simultaneous_rows_or_columns_in_reveal = 2;
Screen screen(MATRIX_WIDTH, MATRIX_HEIGHT);

ThreeWire wire_setup(RTS_DATA_PIN, RTS_CLOCK_PIN, RTS_RESET_PIN); // DAT, CLK, RST
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

  rtc.Begin();

  Serial.begin(115200);
  Serial1.begin(115200);

  screen.clear(true);
  update_screen();
}

bool check_remote_input(SerialCommunicator& communicator) {
  if (communicator.try_read_header())
    return communicator.read_message() == SerialCommunicator::ReadResult::Ok;
  return false;
}

void loop() {
  static bool remote_input_mode = true;
  static bool two_line_layout = true;
  static bool run_seconds_animation = true;
  static uint8_t y_origin_line_1;
  static uint8_t y_origin_line_2 = 8;

  static SerialCommunicator communicator(Serial1);
  static Clock clock(&screen, CHAR_SPACING);
  static Animator animator(&screen);
  static GameOfLife game_of_life(&screen);
  static TextScroller scroller(&screen);
  static RevealTextAnimation revealer(&screen);
  static DisplayMode current_mode = DisplayMode::CLOCK;
  static DisplayMode new_mode = DisplayMode::CLOCK;
  static bool first = true;

  // Set-up clock
  if (first) {
    clock.init(&rtc, RtcDateTime(__DATE__, __TIME__));
    clock.run_reveal_text_animation = true;
    first = false;
  }

  bool new_input = false;
  // Check if any new input is available
  if (remote_input_mode && check_remote_input(communicator)) {
    Message::Type type = communicator.get_current_message()->get_type();
    switch (type) {
      case Message::Type::AnimationFrames:
        new_mode = DisplayMode::ANIMATOR;
        break;
      case Message::Type::GameOfLifeConfig:
        new_mode = DisplayMode::GAME_OF_LIFE;
        break;
      case Message::Type::DotMatrixCommand:
        switch (static_cast<DotMatrixCommandMessage*>(communicator.get_current_message())->cmd) {
          case DotMatrixCommandMessage::Command::SetClockMode:
            new_mode = DisplayMode::CLOCK;
            break;
          case DotMatrixCommandMessage::Command::SetOneLineMode:
            two_line_layout = false;
            break;
          case DotMatrixCommandMessage::Command::SetTwoLineMode:
            two_line_layout = true;
            break;
          case DotMatrixCommandMessage::Command::SetSecondAnimationActive:
            run_seconds_animation = true;
            break;
          case DotMatrixCommandMessage::Command::SetSecondAnimationInactive:
            run_seconds_animation = false;
            break;
          case DotMatrixCommandMessage::Command::ScrollDate:
            // TODO
            //if (two_line_layout)
            //  new_mode = DisplayMode::SCROLL_TEXT_TWO_LINE;
            //scroller.init(clock.string_to_scroll(), scroll_direction, CHAR_SPACING, two_line_layout ? y_origin_line_2 : y_origin_line_1, false);
            break;
        }
        communicator.flush_current_message();
        break;
    }
    new_input = true;
  }

  y_origin_line_1 = two_line_layout ? 1 : 4;
  clock.run_seconds_animation = run_seconds_animation;
  clock.y_origin = y_origin_line_1;
  clock.use_small_font = two_line_layout;

  // Check if the current mode must be terminated
  if (new_input) {
    switch (current_mode) {
      case DisplayMode::REVEAL_TEXT_ANIMATION:
        revealer.terminate();
        break;

      case DisplayMode::SCROLL_TEXT_ANIMATION:
        scroller.terminate();
        break;

      case DisplayMode::ANIMATOR:
        animator.terminate();
        break;

      case DisplayMode::GAME_OF_LIFE:
        game_of_life.terminate();
        break;
    }
  }
  else {
    // Execute the current step
    switch (current_mode) {
      case DisplayMode::CLOCK:
        clock.tick();
        if (clock.wants_mode_change()) {
          new_mode = clock.new_mode();
          if (new_mode == DisplayMode::REVEAL_TEXT_ANIMATION)
            revealer.init(clock.string_to_reveal(), CHAR_SPACING, number_of_simultaneous_rows_or_columns_in_reveal, y_origin_line_1, two_line_layout, reveal_mode);
          else if (new_mode == DisplayMode::SCROLL_TEXT_ANIMATION) {
            if (two_line_layout)
              new_mode = DisplayMode::SCROLL_TEXT_TWO_LINE;
            scroller.init(clock.string_to_scroll(), scroll_direction, CHAR_SPACING, two_line_layout ? y_origin_line_2 : y_origin_line_1, false);
          }
        }
        break;

      case DisplayMode::REVEAL_TEXT_ANIMATION:
        revealer.tick();
        if (revealer.is_done())
          new_mode = DisplayMode::CLOCK;
        break;

      case DisplayMode::SCROLL_TEXT_ANIMATION:
        scroller.tick();
        if (scroller.is_done()) {
          clock.run_reveal_text_animation = true;
          new_mode = DisplayMode::CLOCK;
        }
        break;

      case DisplayMode::ANIMATOR:
        animator.tick();
        if (animator.is_done()) {
          clock.run_reveal_text_animation = true;
          new_mode = DisplayMode::CLOCK;
        }
        break;

      case DisplayMode::GAME_OF_LIFE:
        game_of_life.tick();
        if (game_of_life.is_done()) {
          clock.run_reveal_text_animation = true;
          new_mode = DisplayMode::CLOCK;
        }
        break;

      case DisplayMode::SCROLL_TEXT_TWO_LINE:
        scroller.tick();
        clock.tick();
        if (scroller.is_done())
          new_mode = DisplayMode::CLOCK;
    }
  }

  update_screen();

  // Set-up up the next iteration if a mode change is pending
  if (new_input || (new_mode != current_mode)) {
    switch (new_mode) {
      case DisplayMode::ANIMATOR:
        animator.init(static_cast<AnimationFramesMessage*>(communicator.get_current_message()));
        communicator.flush_current_message();
        break;

      case DisplayMode::GAME_OF_LIFE:
        GameOfLifeConfigMessage* game_of_life_config = static_cast<GameOfLifeConfigMessage*>(communicator.get_current_message());
        game_of_life.init(game_of_life_config);
        communicator.flush_current_message();
        delete(game_of_life_config);
        break;
    }
    current_mode = new_mode;
    update_screen();
  }
}
