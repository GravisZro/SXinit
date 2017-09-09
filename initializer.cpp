#include "initializer.h"

// POSIX
#include <sys/stat.h>

// POSIX++
#include <climits>

// STL
#include <string>
#include <unordered_map>
#include <list>
#include <algorithm>

// PDTK
#include <object.h>
#include <process.h>
#include <cxxutils/hashing.h>
#include <specialized/mount.h>

// Project
#include "fstable.h"
#include "display.h"

#ifndef CONFIG_SERVICE
#define CONFIG_SERVICE      "sxconfig"
#endif

#ifndef EXECUTOR_SERVICE
#define EXECUTOR_SERVICE    "sxexecutor"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME     "config"
#endif

#ifndef EXECUTOR_USERNAME
#define EXECUTOR_USERNAME   "executor"
#endif

#ifndef BIN_PATH
#define BIN_PATH            "/sbin"
#endif

#ifndef DEV_PATH
#define DEV_PATH            "/dev"
#endif

#ifndef PROC_PATH
#define PROC_PATH           "/proc"
#endif

#ifndef MCFS_PATH
#define MCFS_PATH           "/mc"
#endif


#ifndef MCFS_BIN
#define MCFS_BIN            BIN_PATH "/mcfs"
#endif

#ifndef CONFIG_BIN
#define CONFIG_BIN          BIN_PATH "/" CONFIG_SERVICE
#endif

#ifndef EXECUTOR_BIN
#define EXECUTOR_BIN        BIN_PATH "/" EXECUTOR_SERVICE
#endif

#ifndef MCFS_ARGS
#define MCFS_ARGS           MCFS_BIN, MCFS_PATH, "-f", "-o", "allow_other"
#endif

#ifndef CONFIG_ARGS
#define CONFIG_ARGS         CONFIG_BIN, "-f"
#endif

#ifndef EXECUTOR_ARGS
#define EXECUTOR_ARGS       EXECUTOR_BIN, "-f"
#endif

#ifndef CONFIG_SOCKET
#define CONFIG_SOCKET       "/" CONFIG_USERNAME "/io"
#endif

#ifndef EXECUTOR_SOCKET
#define EXECUTOR_SOCKET     "/" EXECUTOR_USERNAME "/io"
#endif



namespace Initializer
{
  bool m_have_procfs;
  bool m_have_mcfs;
  std::string m_procfs_mountpoint;
  std::string m_mcfs_mountpoint;
  std::unordered_map<const char*, Process> m_procs;
}

void Initializer::start(void)
{
  Display::clearItems();
  Display::setItemsLocation(10, 1);
  Display::setItem("MCFS", 0, 0);
  Display::setItem("Config Service", 1, 0);
  Display::setItem("Executor Service", 2, 0);

  Display::setItemState("MCFS", "", "Waiting ");
  Display::setItemState("Config Service", "", "Waiting ");
  Display::setItemState("Executor Service", "", "Waiting ");

  m_have_procfs       = false;
  m_have_mcfs         = false;
  m_procfs_mountpoint = PROC_PATH;
  m_mcfs_mountpoint   = MCFS_PATH;

  if(parse_fstab() == posix::success_response) // parse fstab
  {
    for(fsentry_t& entry : g_fstab) // find specific VFSes
    {
      switch(hash(entry.device))
      {
        case "proc"_hash: // if entry is for proc
          m_have_procfs = true;
          m_procfs_mountpoint = entry.path;
          break;

        case "mcfs"_hash: // if entry is for mcfs
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

    for(fsentry_t& entry : g_fstab) // mount pure VFSes and local devices
    {
      int rval = posix::error_response;
      if(entry.device == entry.filesystems || // pure VFS
         entry.device.find_first_of(DEV_PATH "/") == 0) // block device
        rval = mount(entry.device.c_str(),
                     entry.path.c_str(),
                     entry.filesystems.c_str(),
                     entry.options.c_str());
      if(entry.device == "mcfs") // for MCFS entries
      {
        if(rval == posix::success_response) // mounted MCFS with kernel driver
        {
          Display::setItemState("MCFS", terminal::style::brightGreen, " Passed ");
#if defined(WANT_CONFIG_SERVICE)
          Object::singleShot(start_config_service); // invoke config service
#else
          Object::singleShot(start_executor_service); // invoke executor service
#endif
        }
        else // kernel driver failed/doesn't exist so try FUSE based MCFS
        {
          Object::singleShot(start_mcfs); // invoke FUSE based MCFS mount
        }
      }
    }
  }
  else
  {
    run_emergency_shell();
  }
}


// MCFS
void Initializer::restart_mcfs(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
  if(m_procs.find(MCFS_BIN) != m_procs.end()) // if process existed once
  {
    m_procs.erase(MCFS_BIN); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_mcfs();
}

void Initializer::start_mcfs(void)
{
  if(m_procs.find(MCFS_BIN) != m_procs.end()) // if process exists
    return; // do not try to start it

  Display::setItemState("MCFS", terminal::style::reset, "Starting");

  Process& mcfs = m_procs[MCFS_BIN];
  if(mcfs.setArguments({MCFS_ARGS}) &&
     mcfs.invoke())
    Object::singleShot(test_mcfs); // test if mcfs daemon is active
}

void Initializer::test_mcfs(void) // waits for the mcfs to appear in the mount table
{
  int retries = 5;
  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(parse_mtab() == posix::success_response) // parsed ok
      for(auto pos = g_mtab.begin(); retries > 0 && pos != g_mtab.end(); ++pos) // iterate newly parsed mount table
        if(pos->device == "mcfs" && pos->path == m_mcfs_mountpoint) // if mcfs is mounted
          retries = INT_MIN; // exit loop
    if(retries > 0)
      Display::setItemState("MCFS", terminal::style::brightYellow, "Retrying");
  }

  if(retries == INT_MIN) // if succeeded
  {
    Display::setItemState("MCFS", terminal::style::brightGreen, " Passed ");
#if defined(WANT_CONFIG_SERVICE)
    Object::connect(m_procs[MCFS_BIN].finished, restart_mcfs); // restart FUSE MCFS if it exits
    Object::singleShot(start_config_service); // start config service
#else
    Object::singleShot(start_executor_service);
#endif
  }
  else // if never succeeded
  {
    Display::setItemState("MCFS", terminal::style::brightRed, " Failed ");
    Display::setItemState("Config Service", terminal::style::brightRed, "Canceled");
    Object::singleShot(start_executor_service); // config service will not work without mcfs, so skip it completely
  }
}

// SXConfig
#if defined(WANT_CONFIG_SERVICE)
void Initializer::restart_config_service(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
  if(m_procs.find(CONFIG_BIN) != m_procs.end()) // if process existed once
  {
    m_procs.erase(CONFIG_BIN); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_config_service();
}

void Initializer::start_config_service(void)
{
  if(m_procs.find(CONFIG_BIN) != m_procs.end()) // if process exists
    return; // do not try to start it

  Display::setItemState("Config Service", terminal::style::reset, "Starting");

  Process& config = m_procs[CONFIG_BIN];
  if(config.setArguments({CONFIG_ARGS}) &&
     config.setUserID(posix::getuserid(CONFIG_USERNAME)) &&
     config.invoke())
    Object::singleShot(test_config_service); // test if config service created a socket
}

void Initializer::test_config_service(void)
{
  struct stat data;
  std::string findpath = m_mcfs_mountpoint + CONFIG_SOCKET;
  int retries = 5;

  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(::stat(findpath.c_str(), &data) == posix::success_response && // stat file on VFS worked AND
       data.st_mode & S_IFSOCK) // it's a socket file
      retries = INT_MIN; // exit loop
    else
      Display::setItemState("Config Service", terminal::style::brightYellow, "Retrying");
  }

  if(retries == INT_MIN) // if succeeded
  {
    Display::setItemState("Config Service", terminal::style::brightGreen, " Passed ");
    Object::connect(m_procs[CONFIG_BIN].finished, restart_config_service); // restart config service if it exits
    Object::singleShot(start_executor_service); // start executor service
  }
  else // if never succeeded
  {
    Display::setItemState("Config Service", terminal::style::brightRed, " Failed ");
    Object::singleShot(start_executor_service); // config service seems to have failed to start, ignore it
  }
}
#endif


// SXExecutor
void Initializer::restart_executor_service(posix::fd_t fd, EventData_t data)
{
  (void)fd, (void)data;
  if(m_procs.find(EXECUTOR_BIN) != m_procs.end()) // if process existed once
  {
    m_procs.erase(EXECUTOR_BIN); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_executor_service();
}

void Initializer::start_executor_service(void)
{
  if(m_procs.find(EXECUTOR_BIN) != m_procs.end()) // if process exists
    return; // do not try to start it

  Display::setItemState("Executor Service", terminal::style::reset, "Starting");

  Process& executor = m_procs[EXECUTOR_BIN];
  if(executor.setArguments({EXECUTOR_ARGS}) &&
     executor.setUserID(posix::getuserid(EXECUTOR_USERNAME)) &&
     executor.invoke())
    Object::singleShot(test_executor_service); // test if executor service created a socket
  else
  {
    Display::setItemState("Executor Service", terminal::style::brightRed, " Failed ");
    run_emergency_shell();
  }
}

void Initializer::test_executor_service(void)
{
  struct stat data;
  std::string findpath = m_mcfs_mountpoint + EXECUTOR_SOCKET;
  int retries = 5;

  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(::stat(findpath.c_str(), &data) == posix::success_response && // stat file on VFS worked AND
       data.st_mode & S_IFSOCK) // it's a socket file
      retries = INT_MIN; // exit loop!
    else
      Display::setItemState("Executor Service", terminal::style::brightYellow, "Retrying");
  }

  if(retries == INT_MIN) // if succeeded
  {
    Display::setItemState("Executor Service", terminal::style::brightGreen, " Passed ");
    Object::connect(m_procs[EXECUTOR_BIN].finished, restart_executor_service); // restart executor service if it exits
  }
  else // if never succeeded
  {
    Display::setItemState("Executor Service", terminal::style::brightRed, " Failed ");
    Object::singleShot(run_emergency_shell); // run emergency shell
  }
}


// Desperation
void Initializer::run_emergency_shell(void)
{

  terminal::write("Starting rescue shell!");
}
