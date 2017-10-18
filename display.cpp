#include "display.h"

// POSIX
#include <unistd.h>

// POSIX++
#include <cstring>
#include <cstdio>

// STL
#include <vector>
#include <map>

// PDTK
#include <cxxutils/vterm.h>
#include <cxxutils/hashing.h>

// Project
#ifdef WANT_SPLASH
#include "splash.h"
#include "framebuffer.h"
#endif

namespace Display
{
  static bool kernel_called = ::getpid() == 1;
#ifdef WANT_SPLASH
  static Framebuffer fb;
#endif
  constexpr posix::size_t maxRows = 10;
  constexpr posix::size_t maxColumns = 10;

  static uint16_t screenRows = 0;
  static uint16_t screenColumns = 0;
  static uint16_t offsetRows = 0;
  static uint16_t offsetColumns = 0;
  static std::map<const char*, std::pair<uint16_t, uint16_t>> itempos;
  static std::array<std::array<const char*, maxRows>, maxColumns> items;
  static std::array<size_t, maxColumns> item_column_widths;

  constexpr uint16_t getColumnOffset(uint16_t column) noexcept
    { return !column ? 0 : item_column_widths.at(column - 1) + 16 + getColumnOffset(column - 1); }
}

void Display::init(void) noexcept
{
  clearItems();

  terminal::hideCursor();
  terminal::clearScreen();
  if(true || kernel_called)
  {
#ifdef WANT_SPLASH
    fb.open("/dev/fb0");
    fb.load(data, width, height);
#else
    terminal::getWindowSize(screenRows, screenColumns);
    terminal::setCursorPosition(1, 1);
    terminal::write(CSI "0;47;30m");// reset; white background; black foreground
    for(uint16_t pos = 0; pos < screenColumns;)
      pos += terminal::write(" ");
    setText(1, (screenColumns - sizeof("SYSTEM X")) / 2, "", "SYSTEM X"); // print in the middle of the line
    terminal::write(terminal::style::reset); // reset
#endif
  }
  else
    setText(1, 1, terminal::severe, "System X Initializer is intended to be directly invoked by the kernel.");

  setItemsLocation(10, 4);
}


void Display::setText(uint16_t row, uint16_t column, const char* style, const char* text) noexcept
{
  terminal::setCursorPosition(row, column);
  terminal::write("%s%s", style, text);
  terminal::write("\n");
}

void Display::clearItems(void) noexcept
{
  offsetRows = 0;
  offsetColumns = 0;
  for(auto& rowitems : items)
    rowitems.fill(nullptr);
  item_column_widths.fill(0);
  itempos.clear();
}

bool Display::setItemsLocation(uint16_t row, uint16_t column) noexcept
{
  if(row >= maxRows || column == maxColumns)
    return false;
  offsetRows = row;
  offsetColumns = column;
  return true;
}

bool Display::addItem(const char* item) noexcept
{
  uint16_t column = 0;
  uint16_t row = 0;
  while(items.at(column).at(row) != nullptr)
  {
    if(row == maxRows)
    {
      row = 0;
      ++column;
    }
    else
      ++row;
    if(column == maxColumns)
      return false;
  }
  return setItem(item, row, column);
}

bool Display::setItem(const char* item, uint16_t row, uint16_t column) noexcept
{
  if(row >= maxRows || column == maxColumns)
    return false;
  size_t len = std::strlen(item);
  size_t& current = item_column_widths.at(column);
  if(len > current)
    current = len;

  items.at(column).at(row) = item;
  return itempos.emplace(item, std::make_pair(row, column)).second;
}

bool Display::setItemState(const char* item, const char* style, const char* state) noexcept
{
  auto pos = itempos.find(item);
  if(pos == itempos.end())
    return false;

  uint16_t rowpos = offsetRows + pos->second.first + 1;
  uint16_t colpos = offsetColumns + getColumnOffset(pos->second.second) + 1;
  terminal::write(terminal::style::reset); // reset

  terminal::setCursorPosition(rowpos, colpos);
  terminal::write(item);
  colpos += item_column_widths.at(pos->second.second) + 2;

  terminal::setCursorPosition(rowpos, colpos);
  terminal::write("[        ]");
  ++colpos;

  terminal::setCursorPosition(rowpos, colpos);
  terminal::write(style);
  terminal::write(state);
  terminal::write(terminal::style::reset); // reset
  terminal::write("\n");
  return true;
}

void Display::bailoutLine(const char* fmt, const char* arg1, const char* arg2, const char* arg3)
{
  terminal::setCursorPosition(screenRows - 5, 0);
  terminal::write("%s", terminal::critical);
  terminal::write(fmt, arg1, arg2, arg3);
  terminal::write(terminal::style::reset); // reset
  terminal::write("\n");
}
