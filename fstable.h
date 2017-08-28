#ifndef FSTABLE_H
#define FSTABLE_H

#include <list>
#include <string>

struct fsentry_t
{
  std::string device;
  std::string path;
  std::string filesystems;
  std::string options;
  std::string dump_frequency;
  std::string fsck_pass;

  fsentry_t(void) { }
  fsentry_t(const char* dev ,
            const char* path,
            const char* fs  ,
            const char* opt  = "defaults",
            const char* df   = "0",
            const char* fsck = "0")
    : device(dev),
      path(path),
      filesystems(fs),
      options(opt),
      dump_frequency(df),
      fsck_pass(fsck)
  { }
};


int parse_fstab(void);
int parse_mtab(void);

extern std::list<struct fsentry_t> g_fstab;
extern std::list<struct fsentry_t> g_mtab;

#endif // FSTABLE_H
