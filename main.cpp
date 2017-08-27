// POSIX
#include <unistd.h>

// POSIX++
#include <cstring>
#include <cstdio>

// PDTK
#include <application.h>
#include <cxxutils/colors.h>

// Project
//#include "splash.h"
#include "initializer.h"

int main(int argc, char *argv[])
{
  (void)argc,(void)argv;

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

  Application app;
  Initializer init;
  return app.exec(); // run then exit with generic error code
}
