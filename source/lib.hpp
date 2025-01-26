#pragma once

#include <string>
#include <vector>

#include <CL/opencl.hpp>
#include <SFML/System/Vector2.hpp>

/**
 * @brief The core implementation of the executable
 *
 * This class makes up the library part of the executable, which means that the
 * main logic is implemented here. This kind of separation makes it easy to
 * test the implementation for the executable, because the logic is nicely
 * separated from the command-line logic implemented in the main function.
 */
struct library
{
  /**
   * @brief Flat-representation of black/white pixels to draw.
   */
  std::vector<uint8_t> pixbuffer;

  size_t window_width;
  size_t window_height;
  size_t pixel_size;
  size_t row_count;
  size_t column_count;

  cl::Device device;
  cl::Platform platform;
  cl::Context context;
  cl::CommandQueue queue;
  cl::Buffer in_buffer;
  cl::Buffer out_buffer;
  cl::Program program;
  cl::Kernel kernel;

  std::string name;

  /**
   * @brief Initialize the project; Sets up OpenCL context.
   * @param width The width of the window.
   * @param height The height of the window.
   * @param pixel_size The size of "pixel"s drawn.
   */
  library(size_t width, size_t height, size_t pixel_size);

  /**
   * Start the main game loop.
   */
  auto begin() -> void;

  /**
   * Write content into the pixel buffer.
   */
  auto write_buffer(sf::Vector2u pos,
                    size_t width,
                    const std::vector<bool>& data) -> void;

  /**
   * Compute the next state of each pixel as per the rules.
   * In short,
   * 1. For each point apply the kernel (cross-correlation)
   *   [0 1 0
        1 0 1
        0 1 0]
   * 2. If value < 2 or value > 3 for any point then set to 0.
   * 3. Otherwise 1.
   */
private:
  auto step() -> void;
};
