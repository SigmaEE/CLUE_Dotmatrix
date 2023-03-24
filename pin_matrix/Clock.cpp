#include "Clock.h"
#include "DateAndTimeStringBuilder.h"
#include "DotMatrixFont.h"
#include "TextUtilities.h"

Clock::Clock(Screen* screen, uint8_t char_spacing)
  : m_screen(screen),
  m_char_spacing(char_spacing)
  {
    m_bounding_box = {0, 0, 0, 0};
  }

void Clock::init(RtcDS1302<ThreeWire>* rtc, RtcDateTime program_compiled_timestamp) {
  m_rtc = rtc;
  if (!m_rtc->IsDateTimeValid())
    m_rtc->SetDateTime(program_compiled_timestamp);

  if (m_rtc->GetIsWriteProtected())
    m_rtc->SetIsWriteProtected(false);

  if (!m_rtc->GetIsRunning())
    m_rtc->SetIsRunning(true);

  RtcDateTime now = m_rtc->GetDateTime();

  if (now < program_compiled_timestamp)
    m_rtc->SetDateTime(program_compiled_timestamp);

  if (!m_rtc->GetIsWriteProtected())
    m_rtc->SetIsWriteProtected(true);

    m_last_hour = now.Hour();
}

void Clock::tick() {
  RtcDateTime now = m_rtc->GetDateTime();
  m_rtc_error = !now.IsValid();
  
  if (m_rtc_error) {
    m_wants_mode_change = true;
    m_new_mode = DisplayMode::SCROLL_TEXT_ANIMATION;
    return;
  }

  if (m_last_hour != now.Hour()) {
    m_last_hour = now.Hour();
    DateAndTimeStringBuilder::build_date_string(m_date_string, false, now.Year(), now.Month(), now.Day(), now.DayOfWeek());
    m_wants_mode_change = true;
    m_new_mode = DisplayMode::SCROLL_TEXT_ANIMATION;
    return;
  }

  if (now.Minute() != m_last_minute || ((run_seconds_animation || use_small_font) && now.Second() != m_last_second)) {
    if (!m_bounding_box.all_zero())
      m_screen->clear_bounding_box(m_bounding_box, true);

    DateAndTimeStringBuilder::build_time_string(m_time_string, now.Hour(), now.Minute(), now.Second(), m_draw_colon_separator || use_small_font, use_small_font);
    if (run_reveal_text_animation) {
      m_last_minute = now.Minute();
      m_last_second = now.Second();
      m_wants_mode_change = true;
      m_new_mode = DisplayMode::REVEAL_TEXT_ANIMATION;
      run_reveal_text_animation = false;
      return;
    }
    else {
      uint8_t x_origin = TextUtilities::get_x_origin_of_centered_text(m_screen, m_time_string, m_char_spacing, use_small_font);
      m_bounding_box.left = x_origin;
      m_bounding_box.right = x_origin + TextUtilities::get_number_of_columns_for_text(m_time_string, m_char_spacing, use_small_font);
      m_bounding_box.top = y_origin;
      m_bounding_box.bottom = y_origin + DotMatrixFont::get_height_of_character(use_small_font);
      TextUtilities::write_text_at_position(m_screen, m_time_string, x_origin, y_origin, m_char_spacing, use_small_font, TextUtilities::TextMode::NOT_PERSISTENT);
    }

    m_last_minute = now.Minute();
    if (now.Second() != m_last_second)
      m_draw_colon_separator = !m_draw_colon_separator;
    m_last_second = now.Second();
    m_last_hour = now.Hour();
  }

  m_wants_mode_change = false;
}

bool Clock::wants_mode_change() const {
  return m_wants_mode_change;
}

DisplayMode Clock::new_mode() const {
  return m_new_mode;
}

char* Clock::string_to_scroll() const {
  if (m_new_mode != DisplayMode::SCROLL_TEXT_ANIMATION)
    return nullptr;
  return m_rtc_error ? "CLOCK MODULE ERROR - PLEASE RESTART" : m_date_string;
}

char* Clock::string_to_reveal() const {
  return m_time_string;
}