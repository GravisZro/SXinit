#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>

namespace Display
{
  extern void init(void) noexcept;
  extern void setText (uint16_t row, uint16_t column, const char* style, const char* text) noexcept;
  extern void clearItems(void) noexcept;
  extern bool setItemsLocation(uint16_t row, uint16_t column) noexcept;
  extern bool addItem(const char* item) noexcept;
  extern bool setItem(const char* item, uint16_t row, uint16_t column) noexcept;
  extern bool setItemState(const char* item, const char* style, const char* state) noexcept;
}

#endif // DISPLAY_H
