#include <iostream>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "lib.hpp"

auto main() -> int
{
  constexpr int window_size = 512;
  constexpr int cell_size = 8;
  constexpr int symbol_pos = 10;

  auto lib = library(window_size, window_size, cell_size);
  auto const message = "Hello from " + lib.name + "!";
  std::cout << message << '\n';
  /* clang-format off */
  const std::vector<bool> test_symbol = {
      true, false, false, true, //
      true, true, false, true,  //
      true, false, true, true,  //
      true, false, false, true, //
  };
  lib.write_buffer(sf::Vector2u(symbol_pos, symbol_pos), 4, test_symbol);
  lib.begin();
  return 0;
}
