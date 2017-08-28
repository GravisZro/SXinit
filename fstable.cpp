#include "fstable.h"

// POSIX++
#include <cstring>

// PDTK
#include <cxxutils/posix_helpers.h>

std::list<struct fsentry_t> g_fstab;
std::list<struct fsentry_t> g_mtab;

#if defined(__unix__)

int parse_table(std::list<struct fsentry_t>& table, const char* filename)
{
  struct fsentry_t entry;
  table.clear();

  std::FILE* file = posix::fopen(filename, "r");

  if(file == nullptr)
    return posix::error_response;

  posix::ssize_t count = 0;
  posix::size_t size = 0;
  char* line = nullptr;
  while((count = ::getline(&line, &size, file)) != posix::error_response)
  {
    char* comment = std::strchr(line, '#');
    if(comment != nullptr)
      *comment = '\0';

    char* pos = line;

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.device.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.device.push_back(*pos);

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.path.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.path.push_back(*pos);

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.filesystems.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.filesystems.push_back(*pos);

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.options.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.options.push_back(*pos);

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.dump_frequency.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.dump_frequency.push_back(*pos);

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.fsck_pass.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.fsck_pass.push_back(*pos);

    if(!entry.fsck_pass.empty())
      table.push_back(entry);
  }
  ::free(line); // use C free() because we're using C getline()
  line = nullptr;
  return posix::success();
}


int parse_fstab(void)
{ return parse_table(g_fstab, "/etc/fstab"); }

int parse_mtab(void)
{ return parse_table(g_mtab, "/etc/mtab"); }

#else
#error Unsupported platform! >:(
#endif

