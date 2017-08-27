#include "initializer.h"

// POSIX++
#include <climits>

// STL
#include <list>
#include <algorithm>

// PDTK
#include <cxxutils/hashing.h>
#include <specialized/mount.h>

// Project
#include "fstable.h"

#define WANT_PROCFS

#ifndef MCFS_PATH
#define MCFS_PATH "/bin/mcfs"
#endif

#ifndef SXCONFIG_PATH
#define SXCONFIG_PATH "/bin/sxconfig"
#endif

#ifndef SXEXECUTOR_PATH
#define SXEXECUTOR_PATH "/bin/sxexecutor"
#endif


Initializer::Initializer(void)
  : m_have_procfs(false),
    m_have_mcfs(false),
    m_procfs_mountpoint("/proc"),
    m_mcfs_mountpoint("/mc")
{
  if(parse_fstab() == posix::success_response) // parse fstab
  {
    for(fsentry_t& entry : g_fstab) // mount all filesystems that need mounting (at boot)!
    {
      switch(hash(entry.device))
      {
        case "proc"_hash: // if entry is for proc
          m_have_procfs = true;
          m_procfs_mountpoint = entry.path;
          break;

        case "mc"_hash: // if entry is for mcfs
          m_have_mcfs = true;
          m_mcfs_mountpoint = entry.path;
          break;

        default:
          break;
      }
    }

#if defined(WANT_PROCFS)
    ::mkdir(m_procfs_mountpoint.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
    if(!m_have_procfs) // no fstab entry for procfs!
      g_fstab.emplace_back("proc", m_procfs_mountpoint.c_str(), "proc", "defaults");
#endif
    ::mkdir(m_mcfs_mountpoint.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
    if(!m_have_mcfs) // no fstab entry for MCFS!
      g_fstab.emplace_back("mcfs", m_mcfs_mountpoint.c_str(), "mcfs", "defaults");

    for(fsentry_t& entry : g_fstab) // mount all filesystems
    {
      int rval = mount(entry.device.c_str(),
                       entry.path.c_str(),
                       entry.filesystems.c_str(),
                       entry.options.c_str());
      if(entry.device == "mcfs" && entry.path == m_mcfs_mountpoint)
      {
        if(rval == posix::success_response) // mounted MCFS with kernel driver
          Object::singleShot(this, &Initializer::start_sxconfig); // invoke sxconfig daemon
        else // kernel driver failed/doesn't exist so try FUSE based MCFS
          Object::singleShot(this, &Initializer::start_mcfs); // invoke FUSE based MCFS mount
      }
    }
  }
}

void Initializer::restart_mcfs(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
}

void Initializer::restart_sxconfig(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
}

void Initializer::restart_sxexecutor(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
}

void Initializer::start_mcfs(void)
{
  if(m_procs.find(MCFS_PATH) != m_procs.end()) // if process is being restarted
  {
    ::sleep(1);
    m_procs.erase(MCFS_PATH);
  }
  Process& mcfs = m_procs[MCFS_PATH];
  Object::connect(mcfs.finished, this, &Initializer::restart_mcfs); // restart FUSE FS when it exits
  if(mcfs.setArguments({MCFS_PATH, "-f", "-o", "allow_other", "/mc"}) &&
     mcfs.invoke())
  {

  }
}

void Initializer::start_sxconfig(void)
{
  if(m_procs.find(SXCONFIG_PATH) != m_procs.end()) // if process is being restarted
  {
    ::sleep(1);
    m_procs.erase(SXCONFIG_PATH);
  }
  Process& sxconfig = m_procs[SXCONFIG_PATH];
  Object::connect(sxconfig.finished, this, &Initializer::restart_sxconfig);
  if(sxconfig.setExecutable(SXCONFIG_PATH) &&
     sxconfig.setUserID(posix::getuserid("config")) &&
     sxconfig.invoke())
  {

  }
}

void Initializer::start_sxexecutor(void)
{
  if(m_procs.find(SXEXECUTOR_PATH) != m_procs.end()) // if process is being restarted
  {
    ::sleep(1);
    m_procs.erase(SXEXECUTOR_PATH);
  }
  Process& sxexecutor = m_procs[SXEXECUTOR_PATH];
  Object::connect(sxexecutor.finished, this, &Initializer::restart_sxexecutor);
  if(sxexecutor.setExecutable(SXEXECUTOR_PATH) &&
     sxexecutor.setUserID(posix::getuserid("executor")) &&
     sxexecutor.invoke())
  {

  }
}


