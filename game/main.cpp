//> main
#include "he_engine.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
  HeapEngine engine;

  engine.init();

  engine.run();

  engine.cleanup();

  return 0;
}
//< main
