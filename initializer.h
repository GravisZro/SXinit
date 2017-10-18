#ifndef INITIALIZER_H
#define INITIALIZER_H

#include <specialized/eventbackend.h>

namespace Initializer
{
  void start(void) noexcept;
  void run_emergency_shell(void) noexcept;
}

#endif // INITIALIZER_H
