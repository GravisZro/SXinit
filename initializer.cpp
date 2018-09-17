#include "initializer.h"

// safegaurds
#if !defined(WANT_MOUNT_ROOT) && defined(WANT_MODULES)
#pragma message("Modules loaded by the init system are expressly for mounting root.  Not enabling Module loading.")
#undef WANT_MODULES
#endif

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
#include <childprocess.h>
#include <cxxutils/hashing.h>
#include <specialized/mount.h>

#if defined(WANT_MODULES)
# include <specialized/module.h>
#endif

#if defined(WANT_MOUNT_ROOT)
# include <specialized/blockdevices.h>
#endif

// Project
#include "fstable.h"
#include "display.h"

#ifndef CONFIG_SERVICE
#define CONFIG_SERVICE      "sxconfig"
#endif

#ifndef DIRECTOR_SERVICE
#define DIRECTOR_SERVICE    "sxdirector"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME     "config"
#endif

#ifndef DIRECTOR_USERNAME
#define DIRECTOR_USERNAME   "director"
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

#ifndef SCFS_PATH
#define SCFS_PATH           "/svc"
#endif

#ifndef SCFS_BIN
#define SCFS_BIN            SBIN_PATH "/svcfs"
#endif

#ifndef CONFIG_BIN
#define CONFIG_BIN          SBIN_PATH "/" CONFIG_SERVICE
#endif

#ifndef DIRECTOR_BIN
#define DIRECTOR_BIN        SBIN_PATH "/" DIRECTOR_SERVICE
#endif

#ifndef SCFS_ARGS
#define SCFS_ARGS           SCFS_BIN " " SCFS_PATH " -o allow_other"
#endif

#ifndef CONFIG_ARGS
#define CONFIG_ARGS         CONFIG_BIN " -f"
#endif

#ifndef DIRECTOR_ARGS
#define DIRECTOR_ARGS       DIRECTOR_BIN " -f"
#endif

#ifndef CONFIG_SOCKET
#define CONFIG_SOCKET       "/" CONFIG_USERNAME "/io"
#endif

#ifndef DIRECTOR_SOCKET
#define DIRECTOR_SOCKET     "/" DIRECTOR_USERNAME "/io"
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
  static std::unordered_map<const char*, ChildProcess> s_procs;

  enum class State
  {
    Clear,
    Starting,
    Passed,
    Failed,
    Canceled,
    Retrying,
  };

  void addInitStep(string_literal name, Object::fslot_t<State> function, bool fatal) noexcept;
  void setStepState(string_literal step_id, State state) noexcept;

  struct step_t
  {
    string_literal name;
    Object::fslot_t<State> function;
    bool fatal;
    bool have_result;
    State result;
  };
  static std::list<step_t> s_steps;

#if defined(WANT_MODULES)
  State load_modules(void) noexcept;
#endif

#if defined(WANT_MOUNT_ROOT)
  static fsentry_t root_entry;
  static std::map<std::string, std::string> boot_options;
  State mount_root(void) noexcept;
#endif

  struct vfs_mount
  {
    string_literal step_id;
    int rval;
    fsentry_t* fstab_entry;
    fsentry_t defaults;
    bool fatal;
  };

  State read_vfs_paths(void) noexcept;
  State mount_vfs(vfs_mount* vfs) noexcept;

  static std::list<vfs_mount> s_vfses = {
#if defined(WANT_PROCFS)
    { "Mount ProcFS", posix::error_response, nullptr, { "proc", PROCFS_PATH, PROCFS_NAME, PROCFS_OPTIONS }, false },
#endif
#if defined(WANT_SYSFS)
    { "Mount SysFS", posix::error_response, nullptr, { "sysfs", SYSFS_PATH, "sysfs", "defaults" }, false },
#endif
#if defined(WANT_NATIVE_SCFS)
    { "Mount SCFS", posix::error_response, nullptr, { "scfs", SCFS_PATH, "scfs", "defaults" }, false },
#endif
  };

  struct daemon_data_t
  {
    const char* step_id;
    const char* bin;
    const char* arguments;
    const char* username;
    Object::fslot_t<bool> test;
    bool fatal;
  };

  State daemon_run(daemon_data_t* data) noexcept;
  void restart_daemon(daemon_data_t* data) noexcept;
  bool start_daemon(daemon_data_t* data) noexcept;

// TESTS
  // SXConfig
#if defined(WANT_CONFIG_SERVICE)
  static char config_socket_path[PATH_MAX] = { 0 };
  bool test_config_service(void) noexcept
  {
    struct stat data;
    return ::stat(config_socket_path, &data) == posix::success_response && // stat file on VFS worked AND
        data.st_mode & S_IFSOCK ; // it's a socket file
  }
#endif

  // SXDirector
  static char director_socket_path[PATH_MAX] = { 0 };
  bool test_director_service(void) noexcept
  {
    struct stat data;
    return ::stat(director_socket_path, &data) == posix::success_response && // stat file on VFS worked AND
        data.st_mode & S_IFSOCK; // it's a socket file
  }

  // SCFS
#if defined(WANT_FUSE_SCFS)
  char scfs_mountpoint[PATH_MAX] = { 0 };
  bool test_scfs(void) noexcept
  {
    if(parse_mtab() == posix::success_response) // parsed ok
      for(auto pos = g_mtab.begin(); pos != g_mtab.end(); ++pos) // iterate newly parsed mount table
        if(!std::strcmp(pos->device, "scfs")) // if scfs is mounted
        {
          std::strcpy(scfs_mountpoint, pos->path);
          std::strcpy(config_socket_path, scfs_mountpoint);
          std::strcat(config_socket_path, CONFIG_SOCKET);
          std::strcpy(director_socket_path, scfs_mountpoint);
          std::strcat(director_socket_path, DIRECTOR_SOCKET);
          return true;
        }
    return false;
  }
#endif

 static std::list<daemon_data_t> s_daemons = {
#if defined(WANT_FUSE_SCFS)
   { "Mount FUSE SCFS", SCFS_BIN, SCFS_ARGS, nullptr, test_scfs, false },
#endif
#if defined(WANT_CONFIG_SERVICE)
   { "Config Service", CONFIG_BIN, CONFIG_ARGS, CONFIG_USERNAME, test_config_service, false },
#endif
   { "Director Service", DIRECTOR_BIN, DIRECTOR_ARGS, DIRECTOR_USERNAME, test_director_service, true },
 };

}

void Initializer::addInitStep(string_literal name, Object::fslot_t<State> function, bool fatal) noexcept
{
  s_steps.emplace_back(step_t{ name, function, fatal, false, State::Clear });
  Display::addItem(name);
}

void Initializer::setStepState(string_literal step_id, State state) noexcept
{
  switch(state)
  {
    case State::Clear:    Display::setItemState(step_id, terminal::style::reset       , "        "); break;
    case State::Starting: Display::setItemState(step_id, terminal::style::brightWhite , "Starting"); break;
    case State::Passed:   Display::setItemState(step_id, terminal::style::brightGreen , " Passed "); break;
    case State::Failed:   Display::setItemState(step_id, terminal::style::brightRed   , " Failed "); break;
    case State::Canceled: Display::setItemState(step_id, terminal::style::reset       , "Canceled"); break;
    case State::Retrying: Display::setItemState(step_id, terminal::style::brightYellow, "Retrying"); break;
  }
}

void Initializer::start(void) noexcept
{
  enum {
    Read = 0,
    Write = 1,
  };

  posix::fd_t stderr_pipe[2];

  if(!posix::pipe(stderr_pipe) ||
     !posix::dup2(stderr_pipe[Write], STDERR_FILENO))
    terminal::write("%s Unable to redirect stderr: %s", terminal::warning, std::strerror(errno));

  Display::clearItems();
  Display::setItemsLocation(3, 1);

#if defined(WANT_MODULES)
  addInitStep("Load Modules", load_modules, false);
#endif
#if defined(WANT_MOUNT_ROOT)
  addInitStep("Mount Root", mount_root, false);
#endif

  if(!s_vfses.empty()) // being empty is unlikely but possible
  {
    addInitStep("Find Mount Points", read_vfs_paths, false);
    for(vfs_mount& vfs : s_vfses)
      addInitStep(vfs.step_id, [&vfs]() noexcept { return mount_vfs(&vfs); }, vfs.fatal);
  }

  for(daemon_data_t& daemon : s_daemons)
    addInitStep(daemon.step_id, [&daemon]() noexcept { return daemon_run(&daemon); }, daemon.fatal);

  for(auto& step : s_steps)
    setStepState(step.name, step.result);

  for(auto& step : s_steps)
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
      for(vfs_mount& vfs : s_vfses) // iterate all vfses we want
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
  {
    Object::connect(s_procs[data->bin].finished,
        Object::fslot_t<void, pid_t, posix::error_t>([data](pid_t pid, posix::error_t errnum) noexcept { restart_daemon(data); }));
    Object::connect(s_procs[data->bin].killed,
        Object::fslot_t<void, pid_t, posix::signal::EId>([data](pid_t pid, posix::signal::EId sig_id) noexcept { restart_daemon(data); }));
  }
  return retries == INT_MIN ? State::Passed : State::Failed;
}

bool Initializer::start_daemon(daemon_data_t* data) noexcept
{
  if(s_procs.find(data->bin) != s_procs.end()) // if process exists
    return false; // do not try to start it

  setStepState(data->step_id, State::Starting);
  ChildProcess& proc = s_procs[data->bin]; // create process
  return
      (data->arguments == nullptr || proc.setOption("/Process/Arguments", data->arguments)) && // set arguments if they exist
      (data->username  == nullptr || proc.setOption("/Process/User", data->username)) && // set username if provided
      proc.invoke(); // invoke the process
}

void Initializer::restart_daemon(daemon_data_t* data) noexcept
{
  if(s_procs.find(data->bin) != s_procs.end()) // if process existed once
  {
    s_procs.erase(data->bin); // erase old process entry
    ::sleep(1); // safety delay
  }
  start_daemon(data);
}


#if defined(WANT_MODULES)
Initializer::State Initializer::load_modules(void) noexcept
{
  posix::fd_t fd = posix::open("/modules/modules.list", O_RDONLY);

  if(fd == posix::error_response)
  {
    Display::bailoutLine("Unable to open list of modules to load: %s", std::strerror(errno));
    return State::Failed;
  }

  // TODO

  if(false)
  {
    Display::bailoutLine("Unable to load module %s: %s", "xyz", std::strerror(errno));
    return State::Failed;
  }
  return State::Passed;
}

#endif

#if defined(WANT_MOUNT_ROOT)
Initializer::State Initializer::mount_root(void) noexcept
{
  fsentry_t root_entry;
# if defined(WANT_PROCFS)
  ::mkdir(PROCFS_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
  if(mount("proc", PROCFS_PATH, PROCFS_NAME, PROCFS_OPTIONS) == posix::success_response) // temporarily mount procfs
  {
    constexpr posix::size_t cmdlength = 0x2000; // 8KB
    char cmdline[cmdlength + 1] = { 0 }; // 8KB + NUL char

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
        key.push_back(char(::tolower(*pos))); // options must be lowercase

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
  terminal::setCursorPosition(rows - 5, 0);
  terminal::write("Starting rescue shell\n");
  terminal::showCursor();
}
