// app.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"


struct AppVideo {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* window_texture = nullptr;
  int w_pixels = 0;
  int h_pixels = 0;
  double density = 1.0;
};

struct AppInput {
  Vec2 mouse;
  bool mouse_left_down = false;
  bool mouse_right_down = false;
  bool mouse_click = false;
  bool shift_set = false;
  bool ctrl_set = false;

};

enum struct AppMode {
  NORMAL,
  LINE,
  CIRCLE,
  ARC,
  EDIT,
  GEN,
};

struct AppContext {
  AppMode mode = AppMode::NORMAL;
  bool keep_running = true;
};

struct App {
  AppVideo video;
  AppInput input;
  AppContext context;
};

namespace app {
void reset_frame_state(App &app) {
	app.input.mouse_click = false;
}
} // namespace app
