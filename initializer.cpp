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
#include <functional>

// PDTK
#include <object.h>
#include <process.h>
#include <cxxutils/hashing.h>
#include <specialized/mount.h>

#if defined(WANT_MOUNT_ROOT)
# include <specialized/blockdevices.h>
# if defined(WANT_MODULES)
#  include <specialized/module.h>
# endif
#endif

#if !defined(WANT_MOUNT_ROOT) && defined(WANT_MODULES)
#pragma message("Modules loaded by the init system are expressly for mounting root.  Not enabling Module loading.")
#endif

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
#define MCFS_ARGS           MCFS_BIN " " MCFS_PATH " -o allow_other"
#endif

#ifndef CONFIG_ARGS
#define CONFIG_ARGS         CONFIG_BIN " -f"
#endif

#ifndef EXECUTOR_ARGS
#define EXECUTOR_ARGS       EXECUTOR_BIN " -f"
#endif

#ifndef CONFIG_SOCKET
#define CONFIG_SOCKET       "/" CONFIG_USERNAME "/io"
#endif

#ifndef EXECUTOR_SOCKET
#define EXECUTOR_SOCKET     "/" EXECUTOR_USERNAME "/io"
#endif

#ifdef __linux__
# define PROCFS_NAME    "proc"
# define PROCFS_OPTIONS "default"
# define WANT_PROCFS
#else
# define PROCFS_NAME    "procfs"
# define PROCFS_OPTIONS "linux"
#endif


namespace Initializer
{
  std::unordered_map<const char*, Process> m_procs;

  enum class State
  {
    Clear,
    Starting,
    Passed,
    Failed,
    Canceled,
    Retrying,
  };

  void addInitStep(const char* name, std::function<State()> function, bool fatal) noexcept;
  void setStepState(const char* step_id, State state) noexcept;

  struct step_t
  {
    const char* name;
    std::function<State()> function;
    bool fatal;
    bool have_result;
    State result;
  };
  std::list<step_t> steps;

#if defined(WANT_MOUNT_ROOT)
# if defined(WANT_MODULES)
  State load_modules(void) noexcept;
# endif

  static fsentry_t root_entry;
  static std::map<std::string, std::string> boot_options;
  State mount_root(void) noexcept;
#endif

  struct vfs_mount
  {
    const char* step_id;
    int rval;
    fsentry_t* fstab_entry;
    fsentry_t defaults;
    bool fatal;
  };

  State read_vfs_paths(void) noexcept;
  State mount_vfs(vfs_mount* vfs) noexcept;

  std::list<vfs_mount> vfses = {
#if defined(WANT_PROCFS)
    {"Mount ProcFS", posix::error_response, nullptr, { "proc", PROCFS_PATH, PROCFS_NAME, PROCFS_OPTIONS }, false },
#endif
#if defined(WANT_SYSFS)
    {"Mount SysFS", posix::error_response, nullptr, { "sysfs", SYSFS_PATH, "sysfs", "defaults" }, false },
#endif
#if defined(WANT_MCFS)
    {"Mount MCFS", posix::error_response, nullptr, { "mcfs", MCFS_PATH, "mcfs", "defaults" }, false },
#endif
  };

  struct daemon_data_t
  {
    const char* step_id;
    const char* bin;
    const char* arguments;
    const char* username;
    std::function<bool()> test;
    bool fatal;
  };

  State daemon_run(daemon_data_t* data) noexcept;
  void restart_daemon(daemon_data_t* data) noexcept;
  bool start_daemon(daemon_data_t* data) noexcept;

// TESTS
  // SXConfig
#if defined(WANT_CONFIG_SERVICE)
  char config_socket_path[PATH_MAX] = { 0 };
  bool test_config_service(void) noexcept
  {
    struct stat data;
    return ::stat(config_socket_path, &data) == posix::success_response && // stat file on VFS worked AND
        data.st_mode & S_IFSOCK ; // it's a socket file
  }
#endif

  // SXExecutor
  char executor_socket_path[PATH_MAX] = { 0 };
  bool test_executor_service(void) noexcept
  {
    struct stat data;
    return ::stat(executor_socket_path, &data) == posix::success_response && // stat file on VFS worked AND
        data.st_mode & S_IFSOCK; // it's a socket file
  }

  // MCFS
#if defined(WANT_MCFS)
  char mcfs_mountpoint[PATH_MAX] = { 0 };
  bool test_mcfs(void) noexcept
  {
    if(parse_mtab() == posix::success_response) // parsed ok
      for(auto pos = g_mtab.begin(); pos != g_mtab.end(); ++pos) // iterate newly parsed mount table
        if(!std::strcmp(pos->device, "mcfs")) // if mcfs is mounted
        {
          std::strcpy(mcfs_mountpoint, pos->path);
          std::strcpy(config_socket_path, mcfs_mountpoint);
          std::strcat(config_socket_path, CONFIG_SOCKET);
          std::strcpy(executor_socket_path, mcfs_mountpoint);
          std::strcat(executor_socket_path, EXECUTOR_SOCKET);
          return true;
        }
    return false;
  }
#endif

 std::list<daemon_data_t> daemons = {
#if defined(WANT_MCFS)
   { "Mount FUSE MCFS", MCFS_BIN, MCFS_ARGS, nullptr, test_mcfs, false },
#endif
#if defined(WANT_CONFIG_SERVICE)
   { "Config Service", CONFIG_BIN, CONFIG_ARGS, CONFIG_USERNAME, test_config_service, false },
#endif
   { "Executor Service", EXECUTOR_BIN, EXECUTOR_ARGS, EXECUTOR_USERNAME, test_executor_service, true },
 };

}

void Initializer::addInitStep(const char* name, std::function<State()> function, bool fatal) noexcept
{
  steps.emplace_back((step_t){ name, function, fatal, false, State::Clear });
  Display::addItem(name);
}

void Initializer::setStepState(const char* step_id, State state) noexcept
{
  switch(state)
  {
    case State::Clear:
      Display::setItemState(step_id, terminal::style::reset       , "        ");
      break;
    case State::Starting:
      Display::setItemState(step_id, terminal::style::reset       , "Starting");
      break;
    case State::Passed:
      Display::setItemState(step_id, terminal::style::brightGreen , " Passed ");
      break;
    case State::Failed:
      Display::setItemState(step_id, terminal::style::brightRed   , " Failed ");
      break;
    case State::Canceled:
      Display::setItemState(step_id, terminal::style::brightYellow, "Canceled");
      break;
    case State::Retrying:
      Display::setItemState(step_id, terminal::style::brightYellow, "Retrying");
      break;
  }
}

void Initializer::start(void) noexcept
{
  Display::clearItems();
  Display::setItemsLocation(5, 1);

#if defined(WANT_MOUNT_ROOT)
# if defined(WANT_MODULES)
  addInitStep("Load Modules", load_modules, false);
# endif
  addInitStep("Mount Root", mount_root, false);
#endif
  if(!vfses.empty()) // being empty is unlikely but possible
  {
    addInitStep("Find mountpoints", read_vfs_paths, false);
    for(vfs_mount& vfs : vfses)
      addInitStep(vfs.step_id, [&vfs]() { return mount_vfs(&vfs); }, vfs.fatal);
  }

  for(daemon_data_t& daemon : daemons)
    addInitStep(daemon.step_id, [&daemon]() noexcept { return daemon_run(&daemon); }, daemon.fatal);

  for(auto& step : steps)
    setStepState(step.name, step.result);

  for(auto& step : steps)
  {
    if(!step.have_result || step.result == State::Failed)
    {
      step.result = step.function();
      setStepState(step.name, step.result);
      if(step.result == State::Failed && step.fatal)
      {
        run_emergency_shell();
        break;
      }
    }
  }
}

Initializer::State Initializer::read_vfs_paths(void) noexcept
{
  if(parse_fstab() == posix::success_response) // parse fstab
  {
    for(fsentry_t& entry : g_fstab) // iterate all fstab entries
      for(vfs_mount& vfs : vfses) // iterate all vfses we want
        if(!std::strcmp(entry.device, vfs.defaults.device)) // if the fstab entry is the vfs we want
          vfs.fstab_entry = &entry; // copy a pointer to the entry
    return State::Passed;
  }
  else
    Display::bailoutLine("Unable to parse /etc/fstab!: %s", std::strerror(errno));
  return State::Failed;
}

Initializer::State Initializer::mount_vfs(vfs_mount* vfs) noexcept
{
  if(vfs->fstab_entry != nullptr)
  {
    ::mkdir(vfs->fstab_entry->path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
    vfs->rval = mount(vfs->fstab_entry->device, vfs->fstab_entry->path, vfs->fstab_entry->filesystems, vfs->fstab_entry->options);
  }

  if(vfs->rval != posix::success_response) // if not mounted
  {
    ::mkdir(vfs->defaults.path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
    vfs->rval = mount(vfs->defaults.device, vfs->defaults.path, vfs->defaults.filesystems, vfs->defaults.options); // mount to default directory with default options
  }
  return vfs->rval == posix::success_response ? State::Passed : State::Failed;
}

Initializer::State Initializer::daemon_run(daemon_data_t* data) noexcept
{
  if(data->test())
    return State::Canceled;

  int retries = 5;
  for(; retries > 0; --retries, ::sleep(1)) // keep trying and wait a second between tries
  {
    if(start_daemon(data) && data->test())
      retries = INT_MIN + 1; // exit loop
    else
      setStepState(data->step_id, State::Retrying);
  }

  if(retries == INT_MIN) // if succeeded
    Object::connect(m_procs[data->bin].finished, std::function<void(posix::fd_t)>([data](posix::fd_t){ restart_daemon(data); }));
  return retries == INT_MIN ? State::Passed : State::Failed;
}

bool Initializer::start_daemon(daemon_data_t* data) noexcept
{
  if(m_procs.find(data->bin) != m_procs.end()) // if process exists
    return false; // do not try to start it

  setStepState(data->step_id, State::Starting);
  Process& proc = m_procs[data->bin]; // create process
  return
      (data->arguments == nullptr || proc.setOption("/Process/Arguments", data->arguments)) && // set arguments if they exist
      (data->username  == nullptr || proc.setOption("/Process/User", data->username)) && // set username if provided
      proc.invoke(); // invoke the process
}

void Initializer::restart_daemon(daemon_data_t* data) noexcept
{
  if(m_procs.find(data->bin) != m_procs.end()) // if process existed once
  {
    m_procs.erase(data->bin); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_daemon(data);
}


#if defined(WANT_MOUNT_ROOT)
# if defined(WANT_MODULES)
Initializer::State Initializer::load_modules(void) noexcept
{
  posix::fd_t fd = posix::open("/modules/modules.list", O_RDONLY);

  if(fd == posix::error_response)
  {
    Display::bailoutLine("Unable to open list of modules to load: %s", std::strerror(errno));
    return State::Failed;
  }

  if(false)
  {
    Display::bailoutLine("Unable to load module %s: %s", "xyz", std::strerror(errno));
    return State::Failed;
  }
  return State::Passed;
}

# endif

Initializer::State Initializer::mount_root(void) noexcept
{
  fsentry_t root_entry;
# if defined(WANT_PROCFS)
  ::mkdir(PROCFS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
  if(mount("proc", PROCFS_PATH, PROCFS_NAME, PROCFS_OPTIONS) == posix::success_response) // temporarily mount procfs
  {
    constexpr posix::size_t cmdlength = 0x2000; // 8KB
    char cmdline[cmdlength + 1] = {0}; // 8KB + NUL char

    posix::fd_t fd = posix::open(PROCFS_PATH "/cmdline", O_RDONLY); // read /proc/cmdline so we can read boot options
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
        key.push_back(::tolower(*pos)); // options must be lowercase

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

    blockdevices::init(PROCFS_PATH); // probe system partitions (reads /proc/partitions)
    blockdevice_t* root_device = nullptr;
    auto pos = boot_options.find("root");
    if(pos != boot_options.end())
    {
      std::string::size_type offset = pos->second.find('=');
      if(offset == std::string::npos) // not found
      {
        root_device = blockdevices::lookupByPath(pos->second.c_str()); // try to find in detected devices
        if(root_device == nullptr) // if none found
          root_device = blockdevices::probe(pos->second.c_str()); // probe the specified device name
      }
      else // found '='
      {
        std::string prefix = pos->second.substr(0, offset);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
        switch(hash(prefix))
        {
          case "UUID"_hash: // found "root=UUID=XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
            root_device = blockdevices::lookupByUUID(pos->second.substr(offset).c_str());
            break;
          case "LABEL"_hash: // found "root=LABEL=XXXXXXXX"
            root_device = blockdevices::lookupByLabel(pos->second.substr(offset).c_str());
            break;
          default: // found "root=???=XXXXXXXX" but try to find it anyway
            root_device = blockdevices::lookup(pos->second.substr(offset).c_str());
            break;
        }
      }

      if(root_device != nullptr) // found a device
      {
        std::strcpy(root_entry.device, root_device->path); // copy over data
        std::strcpy(root_entry.filesystems, root_device->fstype);
      }
      else
      {
        Display::bailoutLine("Could not find root device using: %s", pos->second.c_str());
        return State::Failed;
      }
    }
    else
    {
      Display::bailoutLine("No root device specified in boot arguments!");
      return State::Failed;
    }

    unmount(PROCFS_PATH); // done with temporary procfs mount
  }
  else
  {
    Display::bailoutLine("Unable to temporarily mount proc to get boot arguments and find devices: %s", std::strerror(errno));
    return State::Failed;
  }
# else
  blockdevices::init(); // probe system partitions
# endif
  if(mount(root_entry.device, "/", root_entry.filesystems, root_entry.options) != posix::success_response) // mount directly on top of Linux rootfs
  {
    Display::bailoutLine("Unable to mount device \"%s\": %s", root_entry.device, std::strerror(errno));
    return State::Failed;
  }
  return State::Passed;
}
#endif

// Desperation
void Initializer::run_emergency_shell(void) noexcept
{
  uint16_t rows = 0;
  uint16_t columns = 0;
  terminal::getWindowSize(rows, columns);
  terminal::setCursorPosition(rows - 10, 0);
  terminal::write("Starting rescue shell\n");
  terminal::showCursor();
}
