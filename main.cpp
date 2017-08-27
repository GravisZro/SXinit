// POSIX
#include <unistd.h>

// POSIX++
#include <cstring>
#include <cstdio>

// PDTK
#include <application.h>
#include <cxxutils/colors.h>

// Project
#ifdef WANT_SPLASH
#include "splash.h"
#include "framebuffer.h"
#endif
#include "initializer.h"

int main(int argc, char *argv[])
{
  (void)argc,(void)argv;

#ifdef WANT_SPLASH
  Framebuffer fb;
  fb.open("/dev/fb0");
#endif

  if(::getpid() != 1)
  {
    std::fprintf(stdout, "%s%s\n", posix::warning, "System X Initializer is intended to be directly invoked by the kernel.");
    std::fflush(stdout);
  }
  else
  {
#ifdef WANT_SPLASH
    fb.load(data, width, height);
#else
    std::fprintf(stdout, "System X\n");
    std::fflush(stdout);
#endif
  }

  Application app;
  Initializer init;
  return app.exec(); // run then exit with generic error code
}
