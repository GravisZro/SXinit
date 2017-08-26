// STL
#include <unordered_map>
#include <list>
#include <string>
#include <algorithm>

// POSIX++
#include <climits>
#include <cstring>
#include <cstdio>

// PDTK
#include <application.h>
#include <process.h>
#include <cxxutils/colors.h>
#include <cxxutils/hashing.h>
#include <specialized/mount.h>

// Project
#include "fstable.h"
#include "splash.h"


#define MCFS_PATH "/bin/mcfs"
#define SXCONFIG_PATH "/bin/sxconfig"
#define SXEXECUTOR_PATH "/bin/sxexecutor"


#define WANT_PROCFS

static std::unordered_map<const char*, Process> procs;

void start_sxconfig(posix::fd_t fd, EventData_t data);
void start_sxexecutor(posix::fd_t fd, EventData_t data);

void start_mcfs(posix::fd_t fd = 0, EventData_t data = EventData_t())
{
  (void)fd,(void)data;
  if(procs.find(MCFS_PATH) != procs.end()) // if process is being restarted
  {
    ::sleep(1);
    procs.erase(MCFS_PATH);
  }
  Process& mcfs = procs[MCFS_PATH];
  Object::connect(mcfs.finished, start_mcfs); // restart FUSE FS when it exits
  if(mcfs.setArguments({MCFS_PATH, "-f", "-o", "allow_other", "/mc"}) &&
     mcfs.invoke())
  {

  }
}

void start_sxconfig(posix::fd_t fd = 0, EventData_t data = EventData_t())
{
  (void)fd,(void)data;
  if(procs.find(SXCONFIG_PATH) != procs.end()) // if process is being restarted
  {
    ::sleep(1);
    procs.erase(SXCONFIG_PATH);
  }
  Process& sxconfig = procs[SXCONFIG_PATH];
  Object::connect(sxconfig.finished, start_sxconfig);
  if(sxconfig.setExecutable(SXCONFIG_PATH) &&
     sxconfig.setUserID(posix::getuserid("config")) &&
     sxconfig.invoke())
  {

  }
}

void start_sxexecutor(posix::fd_t fd, EventData_t data)
{
  (void)fd,(void)data;
  if(procs.find(SXEXECUTOR_PATH) != procs.end()) // if process is being restarted
  {
    ::sleep(1);
    procs.erase(SXEXECUTOR_PATH);
  }
  Process& sxexecutor = procs[SXEXECUTOR_PATH];
  Object::connect(sxexecutor.finished, start_sxexecutor);
  if(sxexecutor.setExecutable(SXEXECUTOR_PATH) &&
     sxexecutor.setUserID(posix::getuserid("executor")) &&
     sxexecutor.invoke())
  {

  }
}


const std::list<std::string> g_boot_runlevel = { "0", "boot" };

int main(int argc, char *argv[])
{
  (void)argc,(void)argv;

#if defined(WANT_PROCFS)
  char mounts_base[PATH_MAX] = { 0 };
  char proc_base[PATH_MAX] = { 0 };
  int proc_mount_result = posix::error_response;
#endif
  char mcfs_base[PATH_MAX] = { 0 };
  int mcfs_mount_result = posix::error_response;

  if(::getpid() != 1)
  {
    std::fprintf(stdout, "%s%s\n", posix::warning, "System X Initializer is intended to be directly invoked by the kernel.");
    std::fflush(stdout);
  }
  else
  {
    std::fprintf(stdout, "System X\n");
    std::fflush(stdout);
  }

  if(parse_fstab() == posix::success_response) // parse fstab
  {
    for(fsentry_t& entry : g_fstab) // mount all filesystems that need mounting (at boot)!
    {
      switch(hash(entry.device))
      {
#if defined(WANT_PROCFS)
        case "proc"_hash: // if entry is for proc
        {
          std::strcpy(proc_base, entry.path.c_str()); // save mount path
          proc_mount_result = mount(entry.device.c_str(),
                                    entry.path.c_str(),
                                    entry.filesystems.c_str(),
                                    entry.options.c_str()) == posix::success_response ? posix::success_response : errno; // try to mount regardless of entry.mount_runlevel
        }
        break;
#endif
        case "mcfs"_hash: // if entry is for mcfs
        {
          std::strcpy(mcfs_base, entry.path.c_str()); // save mount path
          mcfs_mount_result = mount(entry.device.c_str(),
                                    entry.path.c_str(),
                                    entry.filesystems.c_str(),
                                    entry.options.c_str()) == posix::success_response ? posix::success_response : errno; // try to mount regardless of entry.mount_runlevel
        }
        break;
        default:
          if(std::find(g_boot_runlevel.begin(),
                       g_boot_runlevel.end(),
                       entry.mount_runlevel) != g_boot_runlevel.end()) // if mount_runlevel is in g_boot_runlevel
            mount(entry.device.c_str(),
                  entry.path.c_str(),
                  entry.filesystems.c_str(),
                  entry.options.c_str());
          break;
      }
    }
  }

  Application app;

#if defined(WANT_PROCFS)
  if(proc_mount_result == posix::error_response) // no fstab entry for proc!
  {
    std::strcpy(proc_base, "/proc"); // save mount path
    ::mkdir(proc_base, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
    proc_mount_result = mount("proc", proc_base, "proc", "defaults") == posix::success_response ? posix::success_response : errno; // attempt to mount using defaults
  }

  // Proc should be mounted at this point
  if(proc_mount_result == posix::success_response) // if proc was mounted
  {
    std::strcpy(mounts_base, proc_base);
    std::strcat(mounts_base, "/mounts");
    Object::connect(mounts_base, EventFlags::WriteEvent, start_sxconfig); // watch mount table (mtab) for MCFS to be mounted
#else
#pragma message("Watch for filesystem mounts here")
#endif
    if(!std::strlen(mcfs_base))
      std::strcpy(mcfs_base, "/mc"); // save mount path
    Object::connect(mcfs_base, EventFlags::SubCreated, start_sxexecutor); // wait for sxconfig to use MCFS

    if(mcfs_mount_result == posix::success_response) // MCFS was mounted
      start_sxconfig(); // invoke sxconfig daemon
    else if(mcfs_mount_result == posix::error_response) // no fstab entry for MCFS!
    {
      ::mkdir(mcfs_base, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // create directory (if it doesn't exist)
      mcfs_mount_result = mount("mcfs", mcfs_base, "mcfs", "defaults") == posix::success_response ? posix::success_response : errno; // attempt to mount using defaults
    }

    if(mcfs_mount_result != posix::success_response) // if mount failed
      start_mcfs(); // invoke FUSE based MCFS mount

    // NOTE: when MCFS mounts, it will trigger start_sxconfig() (or be called directly if mounted via fstab)
    //       when SXconfig is running it will use MCFS and trigger start_sxexecutor()
#if defined(WANT_PROCFS)
  }
  else // if proc wasn't mounted
  {
    std::fprintf(stdout, "%s%s\n", posix::severe, "The proc filesystem could not be mounted.  Launching emergency shell.");
    std::fflush(stdout);
  }
#endif
  return 0;
  return app.exec(); // run then exit with generic error code
}
