#ifndef FSTABLE_H
#define FSTABLE_H

#include <list>
#include <climits>

struct fsentry_t
{
  char device[PATH_MAX];
  char path[PATH_MAX];
  char filesystems[0x0800];
  char options[0x1000];
  char dump_frequency;
  char pass;

  fsentry_t(void) noexcept;
  fsentry_t(const char* _device,
            const char* _path,
            const char* _filesystems,
            const char* _options        = "defaults",
            const char* _dump_frequency = "0",
            const char* _pass           = "0") noexcept;
};


int parse_table(std::list<struct fsentry_t>& table, const char* filename) noexcept;

int parse_fstab(void) noexcept;
int parse_mtab(void) noexcept;

extern std::list<struct fsentry_t> g_fstab;
extern std::list<struct fsentry_t> g_mtab;

#endif // FSTABLE_H
