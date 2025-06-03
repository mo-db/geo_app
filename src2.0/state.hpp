#pragma once
#include <SDL3/SDL.h>
#include "graphics.hpp"

enum struct AppMode {
  NORMAL,
  LINE,
  CIRCLE,
  ARC,
  EDIT,
  GEN,
};

struct AppDisplay {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* window_texture = nullptr;
  int w_pixels = 0;
  int h_pixels = 0;
  double density = 1.0;
};

struct InputState {
  Vec2 mouse;
  bool mouse_left_down = false;
  bool mouse_right_down = false;
  bool mouse_click = false;
  bool shift_set = false;
  bool ctrl_set = false;

};

struct AppContext {
  AppMode mode = AppMode::NORMAL;
  bool keep_running = true;
};

struct AppState {
  AppDisplay display;
  InputState input;
  AppContext context;
};

namespace app_state {
void frame_reset(AppState &app) {
	app.input.mouse_click = false;
}
} // namespace state
