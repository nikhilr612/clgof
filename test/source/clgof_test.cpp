#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "clgof" ? 0 : 1;
}
