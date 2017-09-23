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
#include <specialized/blockdevices.h>

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
#define SBIN_PATH           "/sbin"
#endif

#ifndef PROCFS_PATH
#define PROCFS_PATH         "/proc"
#endif

#ifndef SYSFS_PATH
#define SYSFS_PATH          "/sys"
#endif

#ifndef DEVFS_PATH
#define DEVFS_PATH          "/dev"
#endif

#ifndef MCFS_PATH
#define MCFS_PATH           "/mc"
#endif

#ifndef MCFS_BIN
#define MCFS_BIN            SBIN_PATH "/mcfs"
#endif

#ifndef CONFIG_BIN
#define CONFIG_BIN          SBIN_PATH "/" CONFIG_SERVICE
#endif

#ifndef EXECUTOR_BIN
#define EXECUTOR_BIN        SBIN_PATH "/" EXECUTOR_SERVICE
#endif

#ifndef MCFS_ARGS
#define MCFS_ARGS           MCFS_BIN, MCFS_PATH, "-o", "allow_other"
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

#ifdef __linux__
#define PROCFS_NAME    "proc"
#define PROCFS_OPTIONS "default"
#define WANT_PROCFS
#else
#define PROCFS_NAME    "procfs"
#define PROCFS_OPTIONS "linux"
#endif

#undef CONCAT
#define CONCAT(x, y) x##y


namespace Initializer
{
  bool m_have_procfs;
  bool m_have_sysfs;
  bool m_have_mcfs;
  std::string m_procfs_mountpoint;
  std::string m_sysfs_mountpoint;
  std::string m_mcfs_mountpoint;
  std::unordered_map<const char*, Process> m_procs;

#if defined(WANT_MOUNT_ROOT)
  static fsentry_t root_entry;
  static std::map<std::string, std::string> boot_options;
#endif
}


void Initializer::start(void)
{
  Display::clearItems();
  Display::setItemsLocation(5, 1);

#if defined(WANT_MODULES)
  Display::addItem("Load Modules");
#endif
#if defined(WANT_PROCFS)
  Display::addItem("Mount ProcFS");
#endif
#if defined(WANT_SYSFS)
  Display::addItem("Mount SysFS");
#endif
#if defined(WANT_MCFS)
  Display::addItem("Mount MCFS");
#endif
  Display::addItem("Mount Root");
  Display::addItem("Config Service");
  Display::addItem("Executor Service");

#if defined(WANT_MODULES)
  Display::setItemState("Load Modules"    , "", "        ");
#endif
#if defined(WANT_PROCFS)
  Display::setItemState("Mount ProcFS"    , "", "        ");
#endif
#if defined(WANT_SYSFS)
  Display::setItemState("Mount SysFS"     , "", "        ");
#endif
#if defined(WANT_MCFS)
  Display::setItemState("Mount MCFS"      , "", "        ");
#endif
#if defined(WANT_MOUNT_ROOT)
  Display::setItemState("Mount Root"      , "", "        ");
#endif
  Display::setItemState("Config Service"  , "", "        ");
  Display::setItemState("Executor Service", "", "        ");

  //  export PATH=/sbin:/usr/sbin:/bin:/usr/bin

  int procfs_rval = posix::error_response;
  int sysfs_rval  = posix::error_response;
  int mcfs_rval   = posix::error_response;

#if defined(WANT_MOUNT_ROOT)
  fsentry_t root_entry;
# if defined(WANT_PROCFS)
  ::mkdir(PROCFS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
  if(true || mount("proc", PROCFS_PATH, PROCFS_NAME, PROCFS_OPTIONS) == posix::success_response)
  {
    constexpr posix::size_t cmdlength = 0x2000; // 8KB
    char cmdline[cmdlength + 1] = {0}; // 8KB + NUL char

    posix::fd_t fd = posix::open(PROCFS_PATH "/cmdline", O_RDONLY);
    posix::ssize_t count = posix::read(fd, cmdline, cmdlength);

    std::string key;
    std::string value;
    key.reserve(cmdlength);
    value.reserve(cmdlength);

    for(char* pos = cmdline; *pos && pos < cmdline + count; ++pos)
    {
      key.clear();
      value.clear();

      while(*pos && std::isspace(*pos))
        ++pos;

      for(; *pos && pos < cmdline + cmdlength && std::isgraph(*pos) && *pos != '='; ++pos)
        key.push_back(*pos);

      if(*pos == '=')
      {
        for(++pos; *pos && pos < cmdline + cmdlength && std::isgraph(*pos); ++pos)
          value.push_back(*pos);
        boot_options.emplace(key, value);
      }
      else
      {
        switch (hash(key))
        {
          case "debug"_hash:
            boot_options.emplace("debug", "yes");
            break;
          case "break"_hash:
            boot_options.emplace("break", "premount");
            break;
          case "noresume"_hash:
            boot_options.emplace("noresume", "premount");
            break;
          case "ro"_hash:
            std::strcpy(root_entry.options, "ro");
            break;
          case "rw"_hash:
            std::strcpy(root_entry.options, "rw");
            break;
          case "fastboot"_hash:
            boot_options.emplace("fsck.mode", "skip");
            break;
          case "forcefsck"_hash:
            boot_options.emplace("fsck.mode", "force");
            break;
          case "fsckfix"_hash:
            boot_options.emplace("fsck.repair", "yes");
            break;

          default:
            break;
        }
      }
    }

    blockdevices::init(PROCFS_PATH);
    blockdevice_t* root_device = nullptr;
    auto pos = boot_options.find("root");
    if(pos != boot_options.end())
    {
      std::string::size_type offset = pos->second.find('=');
      if(offset == std::string::npos) // not found
        root_device = blockdevices::lookupByPath(pos->second.c_str());
      else // found '='
      {
        std::string prefix = pos->second.substr(0, offset);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        switch(hash(prefix))
        {
          case "UUID"_hash:
            root_device = blockdevices::lookupByUUID(pos->second.substr(offset).c_str());
            break;
          case "LABEL"_hash:
            root_device = blockdevices::lookupByPath(pos->second.substr(offset).c_str());
            break;
          default: // no idea what it is but try to find it
            root_device = blockdevices::lookup(pos->second.substr(offset).c_str());
            break;
        }
      }

      if(root_device != nullptr)
      {
        std::strcpy(root_entry.device, root_device->path);
        std::strcpy(root_entry.filesystems, root_device->fstype);
      }
    }

    unmount(PROCFS_PATH);
  }

  ::mkdir("/newroot", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
  mount(root_entry.device, "/newroot", root_entry.filesystems, root_entry.options);
# endif
#endif

  if(parse_fstab() == posix::success_response) // parse fstab
  {
    for(fsentry_t& entry : g_fstab) // find specific VFSes
    {
      switch(hash(entry.device))
      {
#if defined(WANT_PROCFS)
        case compiletime_hash(PROCFS_NAME, sizeof(PROCFS_NAME)): // if entry is for proc
          ::mkdir(entry.path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
          procfs_rval = mount(entry.device, entry.path, entry.filesystems, entry.options);
          break;
#endif
#if defined(WANT_SYSFS)
        case "sysfs"_hash: // if entry is for sysfs
          ::mkdir(entry.path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
          sysfs_rval = mount(entry.device, entry.path, entry.filesystems, entry.options);
          break;
#endif
#if defined(WANT_MCFS)
        case "mcfs"_hash: // if entry is for mcfs
          ::mkdir(entry.path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
          mcfs_rval = mount(entry.device, entry.path, entry.filesystems, entry.options);
          break;
#endif
        default:
          break;
      }
    }

#if defined(WANT_PROCFS)
    if(procfs_rval != posix::success_response)
    {
      ::mkdir(PROCFS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
      procfs_rval = mount("proc", PROCFS_PATH, PROCFS_NAME, PROCFS_OPTIONS);
    }

    if(procfs_rval == posix::success_response)
      Display::setItemState("Mount ProcFS", terminal::style::brightGreen, " Passed ");
    else
      Display::setItemState("Mount ProcFS", terminal::style::brightRed  , " Failed ");
#endif

#if defined(WANT_SYSFS)
    if(sysfs_rval != posix::success_response)
    {
      ::mkdir(SYSFS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
      sysfs_rval = mount("sysfs", SYSFS_PATH, "sysfs", "defaults");
    }

    if(sysfs_rval == posix::success_response)
      Display::setItemState("Mount SysFS", terminal::style::brightGreen, " Passed ");
    else
      Display::setItemState("Mount SysFS", terminal::style::brightRed  , " Failed ");
#endif

#if defined(WANT_MCFS)
    if(mcfs_rval != posix::success_response)
    {
      ::mkdir(MCFS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
      mcfs_rval = mount("mcfs", MCFS_PATH, "mcfs", "defaults");
    }

    if(mcfs_rval == posix::success_response)
    {
      Display::setItemState("Mount MCFS", terminal::style::brightGreen, " Passed ");
#if defined(WANT_CONFIG_SERVICE)
      Object::singleShot(start_config_service);
#else
      Object::singleShot(start_executor_service);
#endif
    }
    else
      Object::singleShot(start_mcfs);
#endif
  }
  else
  {
    run_emergency_shell();
  }
}

#if defined(WANT_MCFS)
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

  Display::setItemState("Mount MCFS", terminal::style::reset, "Starting");

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
        if(!std::strcmp(pos->device, "mcfs") && m_mcfs_mountpoint == pos->path) // if mcfs is mounted
          retries = INT_MIN + 1; // exit loop
    if(retries > 0)
      Display::setItemState("Mount MCFS", terminal::style::brightYellow, "Retrying");
  }

  if(retries == INT_MIN) // if succeeded
  {
    Display::setItemState("Mount MCFS", terminal::style::brightGreen, " Passed ");
#if defined(WANT_CONFIG_SERVICE)
    Object::connect(m_procs[MCFS_BIN].finished, restart_mcfs); // restart FUSE MCFS if it exits
    Object::singleShot(start_config_service); // start config service
#else
    Object::singleShot(start_executor_service);
#endif
  }
  else // if never succeeded
  {
    Display::setItemState("Mount MCFS", terminal::style::brightRed, " Failed ");
    Display::setItemState("Config Service", terminal::style::brightRed, "Canceled");
    Object::singleShot(start_executor_service); // config service will not work without mcfs, so skip it completely
  }
}
#endif

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
      retries = INT_MIN + 1; // exit loop
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
      retries = INT_MIN + 1; // exit loop!
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
