#include "TextUtilities.h"
#include "DotMatrixFont.h"

uint16_t TextUtilities::get_number_of_columns_for_text(char* text, uint8_t character_spacing, bool use_small_font) {
  uint16_t length = 0;
  uint16_t i = 0;
  while (text[i] != '\0') {
    length += DotMatrixFont::get_width_of_character(text[i], use_small_font) + character_spacing;
    i++;
  }
  return length - character_spacing; // Remove trailing spacing  
}

uint8_t TextUtilities::write_character_at_position(Screen* screen, char ch, int16_t x_origin, int16_t y_origin, bool use_small_font, TextUtilities::TextMode text_mode) {
  uint8_t* character_map = DotMatrixFont::get_character(ch, use_small_font);
  uint8_t character_width = DotMatrixFont::get_width_of_character(ch, use_small_font);
  uint8_t character_height = DotMatrixFont::get_height_of_character(use_small_font);

  for (uint8_t i = 0; i < character_width; i++) {
    for (uint8_t j = 0; j < character_height; j++) {
      if ((character_map[i] >> (character_height - 1 - j)) & 0x01) {
        if (text_mode == TextUtilities::TextMode::PERSISTENT_VISIBLE || text_mode == TextUtilities::TextMode::NOT_PERSISTENT)
          screen->set_value_for_pixel(x_origin + i, y_origin + j, Screen::PixelValue::CURRENT);
        
        if (text_mode == TextUtilities::TextMode::PERSISTENT_VISIBLE || text_mode == TextUtilities::TextMode::PERSISTENT_INVISIBLE)
          screen->set_value_for_pixel(x_origin + i, y_origin + j, Screen::PixelValue::PERSISTENT);
      }
    }
  }

  return character_width;
}

void TextUtilities::write_text_at_position(Screen* screen, char* text, int16_t x_origin, int16_t y_origin, uint8_t character_spacing, bool use_small_font, TextUtilities::TextMode text_mode) {
  uint8_t idx = 0;
  uint8_t character_width;
  while (true) {
    if (text[idx] == '\0')
      break;

    character_width = write_character_at_position(screen, text[idx], x_origin, y_origin, use_small_font, text_mode);
    x_origin += character_width + character_spacing;
    
    idx++;
  }
}

uint8_t TextUtilities::get_x_origin_of_centered_text(Screen* screen, char* text, uint8_t character_spacing, bool use_small_font) {
  uint8_t text_length  = get_number_of_columns_for_text(text, character_spacing, use_small_font);
  return (text_length > screen->screen_width()) ? 0 : (screen->screen_width() - text_length) / 2;
}