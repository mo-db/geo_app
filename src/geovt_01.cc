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

static const int gk_window_width = 1920;
static const int gk_window_height = 1080;
static const uint32_t gk_default_fg_color = 0x00000000;
static const uint32_t gk_default_bg_color = 0xFFFFFFFF;

static bool line_has_first_point = false;
static bool circle_has_center = false;

// Basic Objects
struct Point2D {
	double x {};
	double y {};
};

struct Vec2D {
	double x {};
	double y {};
};

enum struct ObjectState {
	HIGHLIGHTED,
	SELECTED,
	GHOSTED,
	NORMAL,
};

// Complex Objects
struct Line2D {
	Point2D p1 {};
	Point2D p2 {};
	uint32_t color = gk_default_fg_color;
	vector<Point2D> is_points {};
	ObjectState state = ObjectState::NORMAL;
	Line2D(Point2D p1) : p1 {p1} {}
	Line2D(Point2D p1, Point2D p2, uint32_t color)
		: p1 {p1}, p2 {p2}, color {color} {}
};

struct Circle2D {
 	Point2D center {};
	double radius {};
	uint32_t color = gk_default_fg_color;
	vector<Point2D> is_points {};
	ObjectState state = ObjectState::NORMAL;
	Circle2D(Point2D center) : center {center} {}
	Circle2D(Point2D center, double radius, uint32_t color)
		: center {center}, radius {radius}, color {color} {}
};

// struct Arc

struct Objects {
	vector<Line2D> lines;
	vector<Circle2D> circles;
	vector<Point2D> global_is_points;
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
	AppMode mode;

	Point2D mouse;
  bool mouse_left_down;
  bool mouse_right_down;
};

int app_init(AppState *app);
void process_events(AppState *app);
void handle_mouse_click(AppState &app, Objects &objects);
void update(AppState &app, Objects &objects);
void draw(AppState &app, Objects &objects);

auto create_line(Objects &objects, Point2D &p0, Point2D &&p1, uint32_t color)
{
	objects.lines.push_back(Line2D{ p0, p1, color }); // CHECK: is this valid?
	cout << "Line Created, " << objects.lines.size() << "lines" << endl;
	return objects.lines.end();
}

auto create_line(Objects &objects, Point2D &p0)
{
	objects.lines.push_back(Line2D{ p0 }); // CHECK: is this valid?
	cout << "Line Created, " << objects.lines.size() << "lines" << endl;
	return objects.lines.end();
}

auto create_circle(Objects &objects, Point2D &center, double &radius, uint32_t color)
{
	objects.circles.push_back(Circle2D{ center, radius, color });
	cout << "Circle Created, " << objects.circles.size() << "circles" << endl;
	return objects.circles.end();
}

auto create_circle(Objects &objects, Point2D &center)
{
	objects.circles.push_back(Circle2D{ center});
	cout << "Circle Created, " << objects.circles.size() << "circles" << endl;
	return objects.circles.end();
}

int main() {
	AppState app;
	Objects objects;
	if (!app_init(&app))
		return 1;

	while(app.keep_running) {
		process_events(&app);
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
							 case AppMode::NORMAL: cout << "normal" << endl;
							 case AppMode::LINE: cout << "line" << endl;
							 case AppMode::CIRCLE: cout << "cricle" << endl;
						 }
						 cout << endl;
					}
					break;
			}
    case SDL_EVENT_MOUSE_MOTION:
      app.mouse.x = SDL_lround(event.motion.x * app.density);
      app.mouse.y = SDL_lround(event.motion.y * app.density);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      app.mouse_left_down = event.button.down;
			handle_mouse_click(app, objects);
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      app.mouse_left_down = event.button.down;
      break;
		}
	}
}

void handle_mouse_click(AppState &app, Objects &objects) {
	// create shapes
  switch (app.mode) {
  case AppMode::NORMAL:
    break;
  case AppMode::LINE:
		if (!line_has_first_point) {
			create_line(objects, app.mouse);
			line_has_first_point = true;
		} else {
			objects.lines.back().p2 = app.mouse;
			line_has_first_point = false;
		}
    break;
	case AppMode::CIRCLE:
		if (!circle_has_center) {
			create_circle(objects, app.mouse);
			circle_has_center = true;
		} else {
			Circle2D &circle = objects.circles.back();
			circle.radius = SDL_sqrt(SDL_pow((circle.center.x - app.mouse.x), 2.0) + 
						SDL_pow((circle.center.x - app.mouse.x), 2.0)); 
			circle_has_center = false;
		}
    break;
}

// in the objects keyword i want to have lines or circles or other stuff
void draw(AppState &app, Objects &objects) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.window_texture, NULL, &pixels, &pitch)) {
		std::fill_n((uint32_t*)pixels, app.w_pixels * app.h_pixels, 0xFF00FFFF);
		SDL_UnlockTexture(app.window_texture);
	}



	SDL_RenderTexture(app.renderer, app.window_texture, NULL, NULL);
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> 
		dt_ms = t2 - t1;
	std::cout << "dt_out: " << dt_ms << std::endl;

	SDL_RenderPresent(app.renderer);


}
