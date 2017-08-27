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
  std::string mount_runlevel;

  fsentry_t(void) { }
  fsentry_t(const char* dev,
            const char* path = "",
            const char* fs   = "",
            const char* opt  = "",
            const char* df   = "",
            const char* mrun = "")
    : device(dev),
      path(path),
      filesystems(fs),
      options(opt),
      dump_frequency(df),
      mount_runlevel(mrun)
  { }
};


int parse_fstab(void);
extern std::list<struct fsentry_t> g_fstab;

#endif // FSTABLE_H
