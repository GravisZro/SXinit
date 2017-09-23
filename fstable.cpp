#include "fstable.h"

// POSIX++
#include <cstring>

// PDTK
#include <cxxutils/posix_helpers.h>

std::list<struct fsentry_t> g_fstab;
std::list<struct fsentry_t> g_mtab;

#if defined(__unix__)

fsentry_t::fsentry_t(void)
{
  std::memset(this, 0, sizeof(fsentry_t));
}

fsentry_t::fsentry_t(const char* _device,
                     const char* _path,
                     const char* _filesystems,
                     const char* _options,
                     const char* _dump_frequency,
                     const char* _pass)
{
  if(std::strlen(_device) < sizeof(fsentry_t::device))
    std::strcpy(device, _device);
  if(std::strlen(_path) < sizeof(fsentry_t::path))
    std::strcpy(path, _path);
  if(std::strlen(_filesystems) < sizeof(fsentry_t::filesystems))
    std::strcpy(filesystems, _filesystems);
  if(std::strlen(_options) < sizeof(fsentry_t::options))
    std::strcpy(options, _options);
  if(std::strlen(_dump_frequency) == 1)
    dump_frequency = _dump_frequency[0];
  if(std::strlen(_pass) == 1)
    pass = _pass[0];
}


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
  char* begin = nullptr;
  while((count = ::getline(&line, &size, file)) != posix::error_response)
  {
    std::memset(&entry, 0, sizeof(fsentry_t));
    char* comment = std::strchr(line, '#');
    if(comment != nullptr)
      *comment = '\0';

    char* pos = line;

    while(*pos && std::isspace(*pos))
      ++pos;

    for(char* field = begin = entry.device;
        *pos && pos < begin + sizeof(fsentry_t::device) &&
        std::isgraph(*pos); ++field, ++pos)
      *field = *pos;
    if(pos > begin + sizeof(fsentry_t::device))
      continue;

    while(*pos && std::isspace(*pos))
      ++pos;

    for(char* field = begin = entry.path;
        *pos && pos < begin + sizeof(fsentry_t::path) &&
        std::isgraph(*pos); ++field, ++pos)
      *field = *pos;
    if(pos > begin + sizeof(fsentry_t::path))
      continue;

    while(*pos && std::isspace(*pos))
      ++pos;

    for(char* field = begin = entry.filesystems;
        *pos && pos < begin + sizeof(fsentry_t::filesystems) &&
        std::isgraph(*pos); ++field, ++pos)
      *field = *pos;
    if(pos > begin + sizeof(fsentry_t::filesystems))
      continue;

    while(*pos && std::isspace(*pos))
      ++pos;

    for(char* field = begin = entry.options;
        *pos && pos < begin + sizeof(fsentry_t::options) &&
        std::isgraph(*pos); ++field, ++pos)
      *field = *pos;
    if(pos > begin + sizeof(fsentry_t::options))
      continue;

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.dump_frequency = *pos;
    ++pos;
    if(!std::isspace(*pos))
      continue;

    while(*pos && std::isspace(*pos))
      ++pos;

    entry.pass = *pos;

    if(std::isgraph(entry.pass))
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

