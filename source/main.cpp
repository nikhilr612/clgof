#include <iostream>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "lib.hpp"

auto main() -> int
{
  constexpr int WINDOW_SIZE = 512;
  constexpr int CELL_SIZE = 8;
  constexpr int SYMBOL_POS = 10;

  auto lib = library(WINDOW_SIZE, WINDOW_SIZE, CELL_SIZE);
  auto const message = "Hello from " + lib.name + "!";
  std::cout << message << '\n';
  /* clang-format off */
  const std::vector<bool> test_symbol = {
      true, false, false, true, //
      true, true, false, true,  //
      true, false, true, true,  //
      true, false, false, true, //
  };
  lib.write_buffer(sf::Vector2u(SYMBOL_POS, SYMBOL_POS), 4, test_symbol);
  lib.begin();
  return 0;
}
