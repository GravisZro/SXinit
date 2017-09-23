#ifndef FSTABLE_H
#define FSTABLE_H

#include <list>
#include <climits>

struct fsentry_t
{
  char device[PATH_MAX];
  char path[PATH_MAX];
  char filesystems[2048];
  char options[4096];
  char dump_frequency;
  char pass;

  fsentry_t(void);
  fsentry_t(const char* _device,
            const char* _path,
            const char* _filesystems,
            const char* _options        = "defaults",
            const char* _dump_frequency = "0",
            const char* _pass           = "0");
};


int parse_table(std::list<struct fsentry_t>& table, const char* filename);

int parse_fstab(void);
int parse_mtab(void);

extern std::list<struct fsentry_t> g_fstab;
extern std::list<struct fsentry_t> g_mtab;

#endif // FSTABLE_H
