// PDTK
#include <application.h>

// Project
#include "initializer.h"
#include "display.h"

int main(int argc, char *argv[])
{
  (void)argc,(void)argv;
  Display::init();
  Application app;
  Initializer::start();
  return app.exec(); // run then exit with generic error code
}
