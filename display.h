#ifndef DISPLAY_H
#define DISPLAY_H

// PDTK
#include <cxxutils/vterm.h>

namespace Display
{
  extern void init(void) noexcept;
  extern void setText (uint16_t row, uint16_t column, string_literal style, const char* text) noexcept;
  extern void clearItems(void) noexcept;
  extern bool setItemsLocation(uint16_t row, uint16_t column) noexcept;
  extern bool addItem(string_literal item) noexcept;
  extern bool setItem(string_literal item, uint16_t row, uint16_t column) noexcept;
  extern bool setItemState(string_literal item, string_literal style, string_literal state) noexcept;

  extern void bailoutLine(string_literal fmt, const char* arg1 = "", const char* arg2 = "", const char* arg3 = "") noexcept;
}

#endif // DISPLAY_H
