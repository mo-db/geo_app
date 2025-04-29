#include <cstdint>
#include <vector>
#include <iostream>
#include <chrono>
#include <cassert>
#include <cstring>
#include <SDL3/SDL.h>

// INFO: use default arguments for functions like create_circle(default color)

#define DEBUG_MODE

using namespace std;

static const int gk_window_width = 1920/4;
static const int gk_window_height = 1080/4;
static const uint32_t gk_default_fg_color = 0x00000000;
static const uint32_t gk_default_bg_color = 0xFFFFFFFF;

static bool line_under_construction = false;
static bool circle_under_construction = false;

// Basic Objects
struct Vec2 {
	double x {};
	double y {};
};

enum struct ObjectState {
	HIGHLIGHTED,
	SELECTED,
	GHOSTED,
	NORMAL,
};

// How should the destructor look here?, i need to delete is_points if possible
struct Line2 {
	Vec2 p1 {};
	Vec2 p2 {};
	uint32_t color = gk_default_fg_color;
	vector<Vec2> is_points {};
	ObjectState state = ObjectState::NORMAL;
	Line2() {}
	Line2(Vec2 p1) : p1 {p1} {}
	Line2(Vec2 p1, Vec2 p2, uint32_t color)
		: p1 {p1}, p2 {p2}, color {color} {}
};

struct Circle2 {
 	Vec2 center {};
	Vec2 circum_point {};
	uint32_t color = gk_default_fg_color;
	vector<Vec2> is_points {};
	ObjectState state = ObjectState::NORMAL;
	Circle2() {}
	Circle2(Vec2 center) : center {center} {}
	Circle2(Vec2 center, Vec2 circum_point, uint32_t color)
		: center {center}, circum_point {circum_point}, color {color} {}
	int radius() const {
		return SDL_sqrt(SDL_pow((center.x - circum_point.x), 2.0) +
														 SDL_pow((center.y - circum_point.y), 2.0));
	}
};

// struct Arc

struct Objects {
	vector<Line2> lines;
	vector<Circle2> circles;
	// snap points are:
	// all shape defining points
	// all snap points, checked for their uniqueness
	vector<Vec2> snap_points;
};

// the application
enum struct AppMode {
  NORMAL,
  LINE,
	CIRCLE,
};

struct AppState {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *window_texture;
	double density;
	uint32_t w_pixels;
	uint32_t h_pixels;
	bool keep_running;
	AppMode mode = AppMode::NORMAL;

	Vec2 mouse;
  bool mouse_left_down;
  bool mouse_right_down;
	bool mouse_click;
};

int app_init(AppState &app);
void process_events(AppState &app, Objects &objects);
void reset_states(AppState &);
void create_objects(AppState &app, Objects &objects);
void graphics(AppState &app, Objects &objects);
void update(AppState &app, Objects &objects);
void draw(AppState &app, Objects &objects);

auto create_line(Objects &objects, Vec2 &p0, Vec2 &&p1, uint32_t color)
{
	objects.lines.push_back(Line2{ p0, p1, color }); // CHECK: is this valid?
	cout << "Line Created, " << objects.lines.size() << "lines" << endl;
	return objects.lines.end();
}

// auto create_line(Objects &objects, Vec2 &p0)
// {
// 	objects.lines.push_back(Line2{ p0 }); // CHECK: is this valid?
// 	cout << "Line Created, " << objects.lines.size() << "lines" << endl;
// 	return objects.lines.end();
// }

auto create_circle(Objects &objects, Vec2 &center,
		Vec2 &circum_point, uint32_t color)
{
	objects.circles.push_back(Circle2{ center, circum_point, color });
	cout << "Circle Created, " << objects.circles.size() << "circles" << endl;
	return objects.circles.end();
}

auto create_circle(Objects &objects, Vec2 &center)
{
	objects.circles.push_back(Circle2{ center});
	cout << "Circle Created, " << objects.circles.size() << "circles" << endl;
	return objects.circles.end();
}

int main() {
	AppState app;
	Objects objects;
	if (!app_init(app))
		return 1;

	while(app.keep_running) {
		reset_states(app);
		process_events(app, objects);
		create_objects(app, objects);
		graphics(app, objects);
		draw(app, objects);
		SDL_Delay(10);
	}
	return 0;
}

int app_init(AppState &app) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app.window = NULL;
	app.renderer = NULL;
  if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures",
				gk_window_width, gk_window_height, SDL_WINDOW_HIGH_PIXEL_DENSITY |
				SDL_WINDOW_MOUSE_CAPTURE, &app.window, &app.renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app.w_pixels = gk_window_width *
		SDL_GetWindowPixelDensity(app.window);
	app.h_pixels = gk_window_height *
		SDL_GetWindowPixelDensity(app.window);

	// texture create with pixels and not window size . retina display scaling
  app.window_texture = SDL_CreateTexture(
			app.renderer, SDL_PIXELFORMAT_XRGB8888,
			SDL_TEXTUREACCESS_STREAMING, 
			app.w_pixels, app.h_pixels);

	if (!app.window_texture) {
    SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
	}
	app.keep_running = true;
  app.mouse.x = 0;
  app.mouse.y = 0;
  app.mouse_left_down = 0;
  app.mouse_right_down = 0;
  app.density = SDL_GetWindowPixelDensity(app.window);
	std::cout << "w_pixels: " << app.w_pixels << std::endl;
	std::cout << "h_pixels: " << app.h_pixels << std::endl;
  return 1;
}

void process_events(AppState &app, Objects &objects) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      app.keep_running = false;
      break;
		case SDL_EVENT_KEY_DOWN:
			switch(event.key.key) {
        case SDLK_ESCAPE:
          if (!event.key.repeat) {
            app.mode = AppMode::NORMAL;
          }
          break;
				case SDLK_C:
          if (!event.key.repeat) {
            app.mode = AppMode::CIRCLE;
          }
					break;
        case SDLK_L:
          if (!event.key.repeat) {
            app.mode = AppMode::LINE;
          }
          break;

				case SDLK_P:
					if (!event.key.repeat) {
						 cout << "circles:" << endl;
						 for (auto circle : objects.circles) {
							 cout << "(" << circle.center.x << ", "
								 << circle.center.y << ")" << endl;
						 }
						 cout << endl;
						 cout << "lines:" << endl;
						 for (auto line : objects.lines) {
							 cout << "(" << line.p1.x << ", " << line.p1.y << ")"
								 << " | " << "(" << line.p2.x << ", " << line.p2.y << ")"
								 << endl;
						 }
						 cout << endl;
						 cout << "app mode: ";
						 switch (app.mode) {
							 case AppMode::NORMAL: cout << "normal" << endl; break;
							 case AppMode::LINE: cout << "line" << endl; break;
							 case AppMode::CIRCLE: cout << "cricle" << endl; break;
						 }
					}
					break;
			}
    case SDL_EVENT_MOUSE_MOTION:
      app.mouse.x = SDL_lround(event.motion.x * app.density);
      app.mouse.y = SDL_lround(event.motion.y * app.density);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      app.mouse_left_down = event.button.down;
			app.mouse_click = true;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      app.mouse_left_down = event.button.down;
      break;
		}
	}
}

void reset_states(AppState &app) {
	app.mouse_click = false;
}

void create_objects(AppState &app, Objects &objects) {
  switch (app.mode) {
  case AppMode::NORMAL:
    break;
  case AppMode::LINE:
    if (app.mouse_click) {
      if (!line_under_construction) {
        objects.lines.push_back(Line2{app.mouse});
        line_under_construction = true;
      } else {
        objects.lines.back().p2 = app.mouse;
        line_under_construction = false;
      }
    } else if (line_under_construction) {
      objects.lines.back().p2 = app.mouse;
    }
    break;
	case AppMode::CIRCLE:
		if (app.mouse_click) {
			if (!circle_under_construction) {
				objects.circles.push_back(Circle2 {app.mouse});
				circle_under_construction = true;
			} else {
				objects.circles.back().circum_point = app.mouse;
				circle_under_construction = false;
			}
		} else if (circle_under_construction) {
			objects.circles.back().circum_point = app.mouse;
		}
    break;
	}
}

void graphics(AppState &app, Objects &objects) {
}

// in the objects keyword i want to have lines or circles or other stuff
void draw(AppState &app, Objects &objects) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.w_pixels * app.h_pixels, 0xFF00FFFF);

		// refactor
		// draw lines
		for (auto line : objects.lines) {
			int x1 = SDL_lround(line.p1.x);
			int y1 = SDL_lround(line.p1.y);
			int x2 = SDL_lround(line.p2.x);
			int y2 = SDL_lround(line.p2.y);

			double dx = x2 - x1;
			double dy = y2 - y1;
			double y;
			int x;
			double m = dy/dx;
			for (x = x1; x < x2; x++) {
				y = m * (double)(x - x1) + (double)y1;
				if (x < app.w_pixels-1 && y < app.h_pixels-1) {
					pixels_locked[x + SDL_lround(y) * app.w_pixels]
						= line.color;
				}
			}
		}

		SDL_UnlockTexture(app.window_texture);
	}
	SDL_RenderTexture(app.renderer, app.window_texture, NULL, NULL);
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> 
		dt_ms = t2 - t1;
	// std::cout << "dt_out: " << dt_ms << std::endl;
	SDL_RenderPresent(app.renderer);
}
