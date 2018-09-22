#include "fstable.h"

// POSIX++
#include <cstring>
#include <cctype>

// PDTK
#include <cxxutils/posix_helpers.h>

std::list<struct fsentry_t> g_fstab;
std::list<struct fsentry_t> g_mtab;

fsentry_t::fsentry_t(void) noexcept
{
  std::memset(this, 0, sizeof(fsentry_t));
}

fsentry_t::fsentry_t(const char* _device,
                     const char* _path,
                     const char* _filesystems,
                     const char* _options,
                     const char* _dump_frequency,
                     const char* _pass) noexcept
{
  std::strncpy(device     , _device     , sizeof(fsentry_t::device      ));
  std::strncpy(path       , _path       , sizeof(fsentry_t::path        ));
  std::strncpy(filesystems, _filesystems, sizeof(fsentry_t::filesystems ));
  std::strncpy(options    , _options    , sizeof(fsentry_t::options     ));

  if(std::strlen(_dump_frequency) == 1)
    dump_frequency = _dump_frequency[0];

  if(std::strlen(_pass) == 1)
    pass = _pass[0];
}

static char* skip_spaces(char* data) noexcept
{
  while(*data && std::isspace(*data))
    ++data;
  return data;
}

template<size_t sz>
static char* read_field(char* pos, char* destination) noexcept
{
  char* field = destination;
  char* begin = destination;
  while(*pos &&
        pos < begin + sz &&
        std::isgraph(*pos))
    *field++ = *pos++;

  return (pos > begin + sz) ? nullptr : pos;
}

int parse_table(std::list<struct fsentry_t>& table, const char* filename) noexcept
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

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::device)>(pos, entry.device);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::path)>(pos, entry.path);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::filesystems)>(pos, entry.filesystems);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);
    pos = read_field<sizeof(fsentry_t::options)>(pos, entry.options);
    if(pos == nullptr)
      continue;

    pos = skip_spaces(pos);

    entry.dump_frequency = *pos;
    ++pos;
    if(!std::isspace(*pos))
      continue;

    pos = skip_spaces(pos);

    entry.pass = *pos;

    if(std::isgraph(entry.pass))
      table.push_back(entry);
  }
  ::free(line); // use C free() because we're using C getline()
  line = nullptr;
  return posix::success();
}


int parse_fstab(void) noexcept
{ return parse_table(g_fstab, "/etc/fstab"); }


#if defined(__linux__) /* Linux */

int parse_mtab(void) noexcept
{ return parse_table(g_mtab, "/etc/mtab"); }

#elif (defined(__APPLE__) && defined(__MACH__)) /* Darwin 7+     */ || \
      defined(__FreeBSD__)                      /* FreeBSD 4.1+  */ || \
      defined(__DragonFly__)                    /* DragonFly BSD */ || \
      defined(__OpenBSD__)                      /* OpenBSD 2.9+  */ || \
      defined(__NetBSD__)                       /* NetBSD 2+     */

int parse_mtab(void) noexcept
{
  return 0;
}

#elif defined(__sun) && defined(__SVR4) // Solaris / OpenSolaris / OpenIndiana / illumos
# error No filesystem table backend code exists in SXinit for Solaris / OpenSolaris / OpenIndiana / illumos!  Please submit a patch!

#elif defined(__minix) // MINIX
# error No filesystem table backend code exists in SXinit for MINIX!  Please submit a patch!

#elif defined(__QNX__) // QNX
// QNX docs: http://www.qnx.com/developers/docs/7.0.0/index.html#com.qnx.doc.neutrino.devctl/topic/about.html
# error No filesystem table backend code exists in SXinit for QNX!  Please submit a patch!

#elif defined(__hpux) // HP-UX
# error No filesystem table backend code exists in SXinit for HP-UX!  Please submit a patch!

#elif defined(_AIX) // IBM AIX
# error No filesystem table backend code exists in SXinit for IBM AIX!  Please submit a patch!

#elif defined(BSD)
# error Unrecognized BSD derivative!

#elif defined(__unix__) || defined(__unix)
# error Unrecognized UNIX variant!

#else
# error This platform is not supported.
#endif
