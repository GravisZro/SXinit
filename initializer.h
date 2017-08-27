#ifndef INITIALIZER_H
#define INITIALIZER_H

// STL
#include <string>
#include <unordered_map>

// PDTK
#include <object.h>
#include <process.h>

class Initializer : public Object
{
public:
  Initializer(void);

private:
  void restart_mcfs(posix::fd_t fd, EventData_t data);
  void restart_sxconfig(posix::fd_t fd, EventData_t data);
  void restart_sxexecutor(posix::fd_t fd, EventData_t data);

  void start_mcfs(void);
  void start_sxconfig(void);
  void start_sxexecutor(void);

  bool m_have_procfs;
  bool m_have_mcfs;
  std::string m_procfs_mountpoint;
  std::string m_mcfs_mountpoint;
  std::unordered_map<const char*, Process> m_procs;
};



#endif // INITIALIZER_H
