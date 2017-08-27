#include "fstable.h"

// POSIX++
#include <cstring>

// PDTK
#include <cxxutils/posix_helpers.h>

std::list<struct fsentry_t> g_fstab;

#if defined(__unix__)

int parse_fstab(void)
{
  struct fsentry_t entry;
  g_fstab.clear();

  FILE* fstab = posix::fopen("/etc/fstab", "r");

  if(fstab == nullptr)
    return errno;

  posix::ssize_t count = 0;
  posix::size_t size = 0;
  char* line = nullptr;
  while((count = ::getline(&line, &size, fstab)) != posix::error_response)
  {
    if(count == posix::error_response)
      return errno;

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

    entry.mount_runlevel.clear();
    for(; *pos && std::isgraph(*pos); ++pos)
      entry.mount_runlevel.push_back(*pos);

    if(!entry.mount_runlevel.empty())
      g_fstab.push_back(entry);
  }
  ::free(line);
  return posix::success();
}


#else
#error Unsupported platform! >:(
#endif

