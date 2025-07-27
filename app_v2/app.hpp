// app.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"

enum struct AppMode {
  NORMAL,
  LINE,
  CIRCLE,
  ARC,
  EDIT,
  GEN,
	UNITY,
};


struct App {

	struct Context {
		AppMode mode = AppMode::NORMAL;
		bool keep_running = true;
	} context;

	struct Video {
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		SDL_Texture* window_texture = nullptr;
		int w_pixels = 0;
		int h_pixels = 0;
		double density = 1.0;
	} video;

	struct Input {
		Vec2 mouse{};
		bool mouse_left_down = false;
		bool mouse_right_down = false;
		bool mouse_click = false;
		bool shift_set = false;
		bool ctrl_set = false;
	} input;
};
