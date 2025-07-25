#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <numbers>
#include <chrono>
#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>

constexpr uint32_t black =				0x00000000;
constexpr uint32_t white =				0x00FFFFFF;
constexpr uint32_t dark_grey =		0x005C5C5C;
constexpr uint32_t light_grey =		0x00C5C5C5;

constexpr uint32_t red =					0x00FF0000;
constexpr uint32_t green =				0x0000FF00;
constexpr uint32_t blue =					0x000000FF;

constexpr uint32_t yellow =				0x00FFFF00;
constexpr uint32_t magenta =			0x00FF00FF;
constexpr uint32_t cyan =					0x0000FFFF;

constexpr uint32_t fg_color = white;
constexpr uint32_t bg_color = black;
constexpr uint32_t conceal_color = dark_grey;
constexpr uint32_t select_color = red;

constexpr uint32_t hl_primary_color = green;
constexpr uint32_t hl_secondary_color = cyan;
constexpr uint32_t hl_tertiary_color = magenta;

constexpr uint32_t special_color = blue;

constexpr const int gk_window_width = 1920/2;
constexpr int gk_window_height = 1080/2;

namespace gk {
constexpr double epsilon = 1e-6;
constexpr double int_epsilon = 0.5;
} // namespace gk

namespace util {
inline void toggle_bool(bool &b) {
	b = b ? false : true;
}
inline bool epsilon_equal(double x, double y) {
	return (x < y + gk::epsilon && x > y - gk::epsilon);
}
} // namespace util



// ### VEC2 ###
struct Vec2 {
  double x{0.}, y{0.};
  Vec2() = default;
  Vec2(const double x, const double y) : x{x}, y{y} {}
  // magnitude
  double mag() { return std::sqrt(x * x + y * y); }
  // normalize
  Vec2 norm() {
    double m = mag();
    return {x / m, y / m};
  }
};
Vec2 operator+(const Vec2 &a, const Vec2 &b) {
	return {a.x + b.x, a.y + b.y};
}
Vec2 operator-(const Vec2 &a, const Vec2 &b) {
	return {a.x - b.x, a.y - b.y};
}
Vec2 operator*(const Vec2 &v, double d) {
	return {v.x * d, v.y * d};
}
Vec2 operator*(double d, const Vec2 &v) {
	return v * d;
}
namespace vec2 {
double dot(const Vec2 &a, const Vec2 &b) {
	return a.x * b.x + a.y * b.y;
}
double distance(const Vec2 &a, const Vec2 &b) {
  return std::sqrt(std::pow(a.x - b.x, 2.0) + std::pow(a.y - b.y, 2.0));
}
bool equal_int_epsilon(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::int_epsilon &&
         std::abs(a.y - b.y) < gk::int_epsilon;
}
bool equal_epsilon(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::epsilon &&
				 std::abs(a.y - b.y) < gk::epsilon;
}
double get_angle(Vec2 P, Vec2 Q) {
	Vec2 v = Q - P;
  double angle = -std::atan2(v.y, v.x); // because 0/0 is up left
	if (angle < 0) { angle += 2 * std::numbers::pi; }
	return angle;
}
} // namespace vec2

// ### LINE2 ###
struct Line2 {
	Vec2 A{}, B{};
	Line2() = default;
	Line2(const Vec2 A, const Vec2 B) : A{A}, B{B} {}
	Vec2 get_a() const { return Vec2 {B.y - A.y, -(B.x - A.x)}; }
	Vec2 get_v() const { return Vec2 {B.x - A.x, B.y - A.y}; }
	double length() const { return vec2::distance(A, B); }
	Vec2 direction() const { return (B - A).norm(); }
};
namespace line2 {
Vec2 project_point(const Line2 &line, const Vec2 &P) {
	Vec2 a = line.get_a();
	double k = ((line.A.x * a.x + line.A.y * a.y) -
							(P.x * a.x + P.y * a.y)) / (a.x * a.x + a.y * a.y);
	return k * a + P;
}

bool point_in_segment_bounds(const Line2 &line, const Vec2 &P) {
  double distance_to_far_endpoint = std::max(vec2::distance(line.A, P),
																						 vec2::distance(line.B, P));
  return distance_to_far_endpoint <= vec2::distance(line.A, line.B);
}

double get_distance_point_to_ray(const Line2 &line, const Vec2 &P) {
	Vec2 a = line.get_a();
	return std::abs((a.x * P.x + a.y * P.y +
									(-a.x * line.A.x - a.y * line.A.y)) / a.mag());
}
double get_distance_point_to_seg(const Line2 &line, const Vec2 &P) {
  Vec2 projected_point = project_point(line, P);
  if (point_in_segment_bounds(line, projected_point)) {
    return get_distance_point_to_ray(line, P);
  } else {
    return std::min(vec2::distance(P, line.A), vec2::distance(P, line.B));
  }
}
} // namespace line2

struct AppVideo {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* window_texture = nullptr;
  int w_pixels = 0;
  int h_pixels = 0;
  double density = 1.0;
};

struct AppInput {
  Vec2 mouse{};
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

int app_init(App &app) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app.video.window = NULL;
	app.video.renderer = NULL;
  if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures",
				gk_window_width, gk_window_height, SDL_WINDOW_HIGH_PIXEL_DENSITY |
				SDL_WINDOW_MOUSE_CAPTURE, &app.video.window, &app.video.renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app.video.w_pixels = gk_window_width *
		SDL_GetWindowPixelDensity(app.video.window);
	app.video.h_pixels = gk_window_height *
		SDL_GetWindowPixelDensity(app.video.window);

	// texture create with pixels and not window size . retina display scaling
  app.video.window_texture = SDL_CreateTexture(
			app.video.renderer, SDL_PIXELFORMAT_XRGB8888,
			SDL_TEXTUREACCESS_STREAMING, 
			app.video.w_pixels, app.video.h_pixels);

	if (!app.video.window_texture) {
    SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
	}
  app.video.density = SDL_GetWindowPixelDensity(app.video.window);
	std::cout << "w_pixels: " << app.video.w_pixels << std::endl;
	std::cout << "h_pixels: " << app.video.h_pixels << std::endl;

  return 1;
}

void set_pixel(App &app, uint32_t *pixel_buf, int x, int y, uint32_t color) {
	if (x >= 0 && y >= 0 && x < app.video.w_pixels && y < app.video.h_pixels) {
		pixel_buf[x + y * app.video.w_pixels] = color;
	}
}

void plot_line(App& app, uint32_t *pixel_buf, const Line2 &line, uint32_t color) {
	int x0 = std::round(line.A.x);
	int y0 = std::round(line.A.y);
	int x1 = std::round(line.B.x);
	int y1 = std::round(line.B.y);

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  for (;;) { /* loop */
    set_pixel(app, pixel_buf, x0, y0, color);
    e2 = 2 * err;
    if (e2 >= dy) { /* e_xy+e_x > 0 */
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) { /* e_xy+e_y < 0 */
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}

int main() {
	App app;
	if (!app_init(app)) {
		return 1;
	}
	while(app.context.keep_running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_QUIT:
				app.context.keep_running = false;
				break;
			}
		}
	}

	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.video.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.video.w_pixels * app.video.h_pixels, bg_color);
		// plot_line(app, );
		SDL_UnlockTexture(app.video.window_texture);
	}
	SDL_RenderTexture(app.video.renderer, app.video.window_texture, NULL, NULL);
	SDL_RenderPresent(app.video.renderer);
}

