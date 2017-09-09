#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <specialized/eventbackend.h>

namespace Initializer
{
  void start(void);

  void restart_mcfs(posix::fd_t fd, EventData_t data);
  void start_mcfs(void);
  void test_mcfs(void);

#if defined(WANT_CONFIG_SERVICE)
  void restart_config_service(posix::fd_t fd, EventData_t data);
  void start_config_service(void);
  void test_config_service(void);
#endif

  void restart_executor_service(posix::fd_t fd, EventData_t data);
  void start_executor_service(void);
  void test_executor_service(void);

  void run_emergency_shell(void);
}

#endif // INITIALIZER_H
