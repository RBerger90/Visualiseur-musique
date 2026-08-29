#pragma once

#include <stdint.h>

const int FONT_WIDTH = 5;
const int FONT_HEIGHT = 7;

// Returns the FONT_HEIGHT rows of character c, top to bottom. In each row,
// the FONT_WIDTH low-order bits represent the columns left to right (1 = lit
// pixel). Lowercase letters are converted to uppercase. A character not
// defined in the font (accents, symbols...) returns a blank glyph: add an
// entry in Font5x7.cpp to support it.
const uint8_t *getGlyphRows(char c);
