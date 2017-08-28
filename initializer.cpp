#include "initializer.h"

// POSIX
#include <sys/stat.h>

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

#ifndef SXCONFIG_SOCKET
#define SXCONFIG_SOCKET "/config/io"
#endif

#ifndef SXEXECUTOR_PATH
#define SXEXECUTOR_PATH "/bin/sxexecutor"
#endif

#ifndef SXEXECUTOR_SOCKET
#define SXEXECUTOR_SOCKET "/executor/io"
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
  if(m_procs.find(MCFS_PATH) != m_procs.end()) // if process existed once
  {
    m_procs.erase(MCFS_PATH); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_mcfs();
}

void Initializer::restart_sxconfig(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
  if(m_procs.find(SXCONFIG_PATH) != m_procs.end()) // if process existed once
  {
    m_procs.erase(SXCONFIG_PATH); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_sxconfig();
}

void Initializer::restart_sxexecutor(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
  if(m_procs.find(SXEXECUTOR_PATH) != m_procs.end()) // if process existed once
  {
    m_procs.erase(SXEXECUTOR_PATH); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_sxexecutor();
}

void Initializer::start_mcfs(void)
{
  if(m_procs.find(MCFS_PATH) != m_procs.end()) // if process exists
    return; // do not try to start it

  Process& mcfs = m_procs[MCFS_PATH];
  if(mcfs.setArguments({MCFS_PATH, "-f", "-o", "allow_other", "/mc"}) &&
     mcfs.invoke())
    Object::singleShot(this, &Initializer::test_mcfs); // test if mcfs daemon is active
}

void Initializer::start_sxconfig(void)
{
  if(m_procs.find(SXCONFIG_PATH) != m_procs.end()) // if process exists
    return; // do not try to start it

  Process& sxconfig = m_procs[SXCONFIG_PATH];
  if(sxconfig.setArguments({SXCONFIG_PATH, "-f"}) &&
     sxconfig.setUserID(posix::getuserid("config")) &&
     sxconfig.invoke())
    Object::singleShot(this, &Initializer::test_sxconfig); // test if sxconfig created a socket
}

void Initializer::start_sxexecutor(void)
{
  if(m_procs.find(SXEXECUTOR_PATH) != m_procs.end()) // if process exists
    return; // do not try to start it

  Process& sxexecutor = m_procs[SXEXECUTOR_PATH];
  if(sxexecutor.setArguments({SXEXECUTOR_PATH, "-f"}) &&
     sxexecutor.setUserID(posix::getuserid("executor")) &&
     sxexecutor.invoke())
    Object::singleShot(this, &Initializer::test_sxexecutor); // test if sxexecutor created a socket
}

void Initializer::test_mcfs(void) // waits for the mcfs to appear in the mount table
{
  int retries = 5;
  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(parse_mtab() != posix::success_response) // parse error
    {
      Object::singleShot(this, &Initializer::start_sxconfig); // start sxconfig
      retries = INT_MIN; // assume it's ok and exit loop
    }
    else // parsed ok
    {
      for(auto pos = g_mtab.begin(); retries > 0 && pos != g_mtab.end(); ++pos) // iterate newly parsed mount table
      {
        if(pos->device == "mcfs" && pos->path == m_mcfs_mountpoint) // if mcfs is mounted
        {
          Object::singleShot(this, &Initializer::start_sxconfig); // start sxconfig
          retries = INT_MIN; // exit loop
        }
      }
    }
  }

  if(retries == INT_MIN) // if succeeded
  {
    Object::connect(m_procs[MCFS_PATH].finished, this, &Initializer::restart_mcfs); // restart FUSE MCFS if it exits
  }
  else if(!retries) // if tried 5 times and never succeeded
  {
    Object::singleShot(this, &Initializer::start_sxexecutor); // sxconfig will not work without mcfs, so skip it completely
  }
}

void Initializer::test_sxconfig(void)
{
  struct stat data;
  std::string findpath = m_mcfs_mountpoint + SXCONFIG_SOCKET;
  int retries = 5;

  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(::stat(findpath.c_str(), &data) == posix::success_response && // stat file on VFS worked AND
       data.st_mode & S_IFSOCK) // it's a socket file
    {
      Object::singleShot(this, &Initializer::start_sxexecutor); // start sxexecutor
      retries = INT_MIN; // exit loop
    }
  }

  if(retries == INT_MIN) // if succeeded then connect make sure it will restart when needed
  {
    Object::connect(m_procs[SXCONFIG_PATH].finished, this, &Initializer::restart_sxconfig); // restart sxconfig if it exits
  }
  else if(!retries) // if tried 5 times and never succeeded
  {
    Object::singleShot(this, &Initializer::start_sxexecutor); // sxconfig seems to have failed to start, ignore it
  }
}

void Initializer::test_sxexecutor(void)
{
  struct stat data;
  std::string findpath = m_mcfs_mountpoint + SXEXECUTOR_SOCKET;
  int retries = 5;

  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(::stat(findpath.c_str(), &data) == posix::success_response && // stat file on VFS worked AND
       data.st_mode & S_IFSOCK) // it's a socket file
    {
      retries = INT_MIN; // exit loop!
    }
  }

  if(retries == INT_MIN) // if succeeded then connect make sure it will restart when needed
  {
    Object::connect(m_procs[SXEXECUTOR_PATH].finished, this, &Initializer::restart_sxexecutor); // restart sxexecutor if it exits
  }
  else if(!retries) // if tried 5 times and never succeeded
  {
    Object::singleShot(this, &Initializer::run_emergency_shell); // run emergency shell
  }
}

void Initializer::run_emergency_shell(void)
{

}
