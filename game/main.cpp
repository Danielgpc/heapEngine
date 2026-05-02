//> main
#include <he_engine.h>

int main(int argc, char *argv[])
{
  HeapEngine engine;

  engine.init();

  engine.run();

  engine.cleanup();

  return 0;
}
//< main
