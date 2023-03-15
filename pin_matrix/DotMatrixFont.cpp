#include "DotMatrixFont.h"

uint8_t DotMatrixFont::get_width_of_character(char ch) {
  switch (ch) {
    case ':':
    case ' ':
      return 1;
    case 'I':
    case '0':
    case '1':
      return 3;
    case 'J':
    case 'M':
    case 'N':
    case 'T':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case '-':
      return 5;
    default: 
      return 4; // Almost all characters are 4 dots wide
  }
}

uint8_t DotMatrixFont::get_height_of_character() {
  return DotMatrixFont::character_height;
}

uint8_t* DotMatrixFont::get_character(char ch) {
  switch (ch) {
    case 'A': return CHAR_A;
    case 'B': return CHAR_B;
    case 'C': return CHAR_C;
    case 'D': return CHAR_D;
    case 'E': return CHAR_E;
    case 'F': return CHAR_F;
    case 'G': return CHAR_G;
    case 'H': return CHAR_H;
    case 'I': return CHAR_I;
    case 'J': return CHAR_J;
    case 'K': return CHAR_K;
    case 'L': return CHAR_L;
    case 'M': return CHAR_M;
    case 'N': return CHAR_N;
    case 'O': return CHAR_O;
    case 'P': return CHAR_P;
    case 'Q': return CHAR_Q;
    case 'R': return CHAR_R;
    case 'S': return CHAR_S;
    case 'T': return CHAR_T;
    case 'U': return CHAR_U;
    case 'V': return CHAR_V;
    case 'W': return CHAR_W;
    case 'X': return CHAR_X;
    case 'Y': return CHAR_Y; 
    case 'Z': return CHAR_Z;
    case '0': return CHAR_0;
    case '1': return CHAR_1;
    case '2': return CHAR_2;
    case '3': return CHAR_3;
    case '4': return CHAR_4;
    case '5': return CHAR_5;
    case '6': return CHAR_6;
    case '7': return CHAR_7;
    case '8': return CHAR_8;
    case '9': return CHAR_9;
    case ':': return CHAR_COLON;
    case ' ': return CHAR_SPACE;
    case '-': return CHAR_HYPHEN;
  }
}

uint8_t DotMatrixFont::to_character_matrix_idx(uint8_t x, uint8_t y) {
  return (x * get_height_of_character()) + y;
}

const uint8_t DotMatrixFont::character_height = 7; // All characters are 7 dots tall

// Characters are stored columnwise, bottom seven bits encode pixel on or off
const uint8_t DotMatrixFont::CHAR_A[] = { 0x3f, 0x48, 0x48, 0x3f };
const uint8_t DotMatrixFont::CHAR_B[] = { 0x7f, 0x49, 0x49, 0x36 };
const uint8_t DotMatrixFont::CHAR_C[] = { 0x3e, 0x41, 0x41, 0x22 };
const uint8_t DotMatrixFont::CHAR_D[] = { 0x7f, 0x41, 0x41, 0x3e };
const uint8_t DotMatrixFont::CHAR_E[] = { 0x7f, 0x49, 0x49, 0x41 };
const uint8_t DotMatrixFont::CHAR_F[] = { 0x7f, 0x48, 0x48, 0x40 };
const uint8_t DotMatrixFont::CHAR_G[] = { 0x3e, 0x41, 0x49, 0x2f };
const uint8_t DotMatrixFont::CHAR_H[] = { 0x7f, 0x08, 0x08, 0x7f };
const uint8_t DotMatrixFont::CHAR_I[] = { 0x41, 0x7f, 0x41 };
const uint8_t DotMatrixFont::CHAR_J[] = { 0x02, 0x01, 0x41, 0x7e, 0x40 };
const uint8_t DotMatrixFont::CHAR_K[] = { 0x7f, 0x08, 0x14, 0x63 };
const uint8_t DotMatrixFont::CHAR_L[] = { 0x7f, 0x01, 0x01, 0x01 };
const uint8_t DotMatrixFont::CHAR_M[] = { 0x7f, 0x20, 0x10, 0x20, 0x7f };
const uint8_t DotMatrixFont::CHAR_N[] = { 0x7f, 0x30, 0x08, 0x06, 0x7f };
const uint8_t DotMatrixFont::CHAR_O[] = { 0x3e, 0x41, 0x41, 0x3e };
const uint8_t DotMatrixFont::CHAR_P[] = { 0x7f, 0x48, 0x48, 0x30 };
const uint8_t DotMatrixFont::CHAR_Q[] = { 0x3c, 0x42, 0x42, 0x3d };
const uint8_t DotMatrixFont::CHAR_R[] = { 0x7f, 0x48, 0x48, 0x37 };
const uint8_t DotMatrixFont::CHAR_S[] = { 0x31, 0x49, 0x49, 0x46 };
const uint8_t DotMatrixFont::CHAR_T[] = { 0x40, 0x40, 0x7f, 0x40, 0x40 };
const uint8_t DotMatrixFont::CHAR_U[] = { 0x7e, 0x01, 0x01, 0x7e };
const uint8_t DotMatrixFont::CHAR_V[] = { 0x78, 0x06, 0x01, 0x06, 0x78 };
const uint8_t DotMatrixFont::CHAR_W[] = { 0x7f, 0x02, 0x04, 0x02, 0x7f };
const uint8_t DotMatrixFont::CHAR_X[] = { 0x63, 0x14, 0x08, 0x14, 0x63 };
const uint8_t DotMatrixFont::CHAR_Y[] = { 0x63, 0x14, 0x08, 0x10, 0x60 };
const uint8_t DotMatrixFont::CHAR_Z[] = { 0x43, 0x45, 0x49, 0x71 };
const uint8_t DotMatrixFont::CHAR_0[] = { 0x3e, 0x41, 0x3e };
const uint8_t DotMatrixFont::CHAR_1[] = { 0x21, 0x7f, 0x01 };
const uint8_t DotMatrixFont::CHAR_2[] = { 0x23, 0x45, 0x49, 0x31 };
const uint8_t DotMatrixFont::CHAR_3[] = { 0x22, 0x41, 0x49, 0x36 };
const uint8_t DotMatrixFont::CHAR_4[] = { 0x18, 0x28, 0x48, 0x7f };
const uint8_t DotMatrixFont::CHAR_5[] = { 0x72, 0x51, 0x51, 0x4e };
const uint8_t DotMatrixFont::CHAR_6[] = { 0x3e, 0x51, 0x51, 0x0e };
const uint8_t DotMatrixFont::CHAR_7[] = { 0x43, 0x44, 0x48, 0x70 };
const uint8_t DotMatrixFont::CHAR_8[] = { 0x36, 0x49, 0x49, 0x36 };
const uint8_t DotMatrixFont::CHAR_9[] = { 0x38, 0x45, 0x45, 0x3e };
const uint8_t DotMatrixFont::CHAR_COLON[] = { 0x12 };
const uint8_t DotMatrixFont::CHAR_SPACE[] = { 0x00 };
const uint8_t DotMatrixFont::CHAR_HYPHEN[] = { 0x08, 0x08, 0x08, 0x08, 0x08 };