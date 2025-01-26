#include <iostream>
#include <stdexcept>
#include <vector>

#include "lib.hpp"

#include <CL/cl.h>
#include <CL/opencl.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

static const char* const kernel_src = R"(
    __kernel void game_of_life(__global const uchar* input,
                              __global uchar* output,
                              const int width,
                              const int height) {
        // Get current cell position
        const int x = get_global_id(0);
        const int y = get_global_id(1);

        // Skip if outside bounds
        if (x >= width || y >= height) return;

        // Calculate current cell index
        const int idx = y * width + x;

        // Count live neighbors
        int neighbors = 0;

        // Check all 8 neighboring cells
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                // Skip the cell itself
                if (dx == 0 && dy == 0) continue;

                // Calculate neighbor coordinates with wrapping
                int nx = (x + dx + width) % width;
                int ny = (y + dy + height) % height;

                // Add to neighbor count if cell is alive
                neighbors += input[ny * width + nx] ? 1 : 0;
            }
        }

        // Apply Conway's Game of Life rules
        bool current_cell = input[idx];
        bool next_state;

        if (current_cell) {
            // Live cell survives if it has 2 or 3 neighbors
            next_state = (neighbors == 2 || neighbors == 3);
        } else {
            // Dead cell becomes alive if it has exactly 3 neighbors
            next_state = (neighbors == 3);
        }

        // Write result to output buffer
        output[idx] = next_state;
    }
)";

/**
 * Find the suitable OpenCL platform and device for computation.
 *
 * Searches for devices in order of preference:
 * 1. GPU devices (preferred for parallel computation)
 * 2. Accelerator devices (specialized hardware)
 * 3. CPU devices (fallback option)
 *
 * Prints diagnostic information about available platforms and devices
 * during the search process.
 *
 * @return std::pair containing the selected platform and device
 * @throws std::runtime_error if no OpenCL devices are found
 */
namespace
{
auto get_platform_device() -> std::pair<cl::Platform, cl::Device>
{
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  if (platforms.empty()) {
    throw std::runtime_error("No OpenCL platforms found");
  }

  // Try to find a GPU device first
  for (const auto& platform : platforms) {
    std::vector<cl::Device> devices;
    try {
      platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
      if (!devices.empty()) {
        std::cout << "Using GPU: " << devices[0].getInfo<CL_DEVICE_NAME>()
                  << "\n"
                  << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>()
                  << "\n";
        return {platform, devices[0]};
      }
    } catch (...) {
      std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>()
                << " has no GPUs\n";
    }
  }

  std::cout << "No GPUs found on any platform. Looking for accelerators.\n";

  // If no GPU found, try to find an accelerator
  for (const auto& platform : platforms) {
    std::vector<cl::Device> devices;
    try {
      platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
      if (!devices.empty()) {
        std::cout << "Using Accelerator: "
                  << devices[0].getInfo<CL_DEVICE_NAME>() << "\n"
                  << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>()
                  << "\n";
        return {platform, devices[0]};
      }
    } catch (...) {
      // Continue searching if this platform has no accelerator devices
      std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>()
                << " has no Accelerators\n";
    }
  }

  std::cout << "No accelerators found on any platform. Fallback to CPU.\n";

  // If no GPU or accelerator found, fall back to CPU
  for (const auto& platform : platforms) {
    std::vector<cl::Device> devices;
    try {
      platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
      if (!devices.empty()) {
        std::cout << "Using CPU: " << devices[0].getInfo<CL_DEVICE_NAME>()
                  << "\n"
                  << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>()
                  << "\n";
        return {platform, devices[0]};
      }
    } catch (...) {
      // Continue searching if this platform has no CPU devices
      std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>()
                << " has no CPU\n";
    }
  }

  throw std::runtime_error("No OpenCL devices found");
}
}  // namespace

library::library(size_t width, size_t height, size_t pixel_size)
    : name("CLGOF")
    , window_width(width)
    , window_height(height)
    , pixel_size(pixel_size)
{
  size_t row_count = width / pixel_size;
  size_t col_count = height / pixel_size;
  pixbuffer = std::vector<uint8_t>(row_count * col_count, 0);
  this->row_count = row_count;
  this->column_count = col_count;

  // Setup OpenCL
  auto [platform, device] = get_platform_device();
  context = cl::Context(device);
  queue = cl::CommandQueue(context, device);

  program = cl::Program(context, kernel_src);
  try {
    program.build({device});
  } catch (...) {
    std::cerr << "OpenCL build error: "
              << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
    throw;
  }

  kernel = cl::Kernel(program, "game_of_life");
  in_buffer = cl::Buffer(
      context, CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY, pixbuffer.size());
  out_buffer = cl::Buffer(
      context, CL_MEM_HOST_READ_ONLY | CL_MEM_WRITE_ONLY, pixbuffer.size());
}

auto library::begin() -> void
{
  sf::RenderWindow window(sf::VideoMode(window_width, window_height), name);

  std::vector<sf::RectangleShape> rects(pixbuffer.size());

  for (size_t i = 0; i < pixbuffer.size(); i++) {
    size_t pos_x = (i % column_count) * pixel_size;
    size_t pos_y = (i / column_count) * pixel_size;
    rects[i].setSize(sf::Vector2f(static_cast<float>(pixel_size),
                                  static_cast<float>(pixel_size)));
    rects[i].setPosition(
        sf::Vector2f(static_cast<float>(pos_x), static_cast<float>(pos_y)));
  }

  while (window.isOpen()) {
    sf::Event event {};
    while (window.pollEvent(event)) {
      switch (event.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::KeyPressed: {
          const auto key_code = event.key.code;
          if (key_code == sf::Keyboard::Key::Enter) {
            std::cout << "Stepping...\n";
            this->step();
            // TODO: Add OpenCL computation here...
          }
          break;
        }
        default:
          break;
      }
    }

    for (size_t i = 0; i < pixbuffer.size(); i++) {
      rects[i].setFillColor(pixbuffer[i] != 0 ? sf::Color::White
                                              : sf::Color::Black);
      window.draw(rects[i]);
    }

    window.display();
  }
}

auto library::write_buffer(sf::Vector2u pos,
                           size_t width,
                           const std::vector<bool>& data) -> void
{
  size_t height = data.size() / width;
  for (size_t rel_x = 0; rel_x < width; rel_x++) {
    for (size_t rel_y = 0; rel_y < height; rel_y++) {
      size_t d_i = rel_x + (rel_y * width);
      size_t b_i = (pos.x + rel_x) + ((pos.y + rel_y) * column_count);
      pixbuffer[b_i] = static_cast<uint8_t>(data[d_i]);
    }
  }
}

auto library::step() -> void
{
  // Write current game state to input buffer
  queue.enqueueWriteBuffer(
      in_buffer, CL_TRUE, 0, pixbuffer.size() * sizeof(bool), pixbuffer.data());

  // Set kernel arguments
  kernel.setArg(0, in_buffer);  // Input buffer (current state)
  kernel.setArg(1, out_buffer);  // Output buffer (next state)
  kernel.setArg(2, static_cast<int>(column_count));  // Grid width
  kernel.setArg(3, static_cast<int>(row_count));  // Grid height

  // Execute kernel on 2D grid
  cl::NDRange global(column_count, row_count);
  queue.enqueueNDRangeKernel(kernel, cl::NullRange, global, cl::NullRange);

  // Read results back into pixbuffer
  queue.enqueueReadBuffer(out_buffer,
                          CL_TRUE,
                          0,
                          pixbuffer.size() * sizeof(bool),
                          pixbuffer.data());
}
