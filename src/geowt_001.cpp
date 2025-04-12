#include <cstdint>
#include <vector>
#include <iostream>
#include <SDL3/SDL.h>

// INFO: use default arguments for functions like create_circle(default color)

#define DEBUG_MODE

using namespace std;

static const int gk_window_height = 640;
static const int gk_window_width = 480;
static const int gk_objects_max = 1000;
static const int gk_text_buf_max = 1024;
static const uint32_t gk_fg_color_default = 0x00000000;
static const uint32_t gk_bg_color_default = 0xFFFFFFFF;

typedef struct {
  double x;
  double y;
} Point2D;

std::vector<Point2D> g_points{}; // empty vector

std::vector<int> g_numbers { 5, 8, 9, 1 }; // list constructor is called
vector<int> g_few_numbers ( 10 ); // explicit constructor to specify length
vector<int> g_few_numbers2 { 10 }; // matches list constructor, lenght 1

template <typename T>
T my_max(T x, T y) {
	return (x < y) ? y : x;
}

typedef enum {
  NORMAL,
  POINT,
  LINE,
	CIRCLE,
} AppMode;

typedef struct {
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
	bool mouse_click;

  // bool text_input;
  // char text[gk_text_buf_max];
} AppState;

int app_init(AppState *app);
void process_event(AppState *app);

int main() {
	AppState app;
	if (!app_init(&app))
		return 1;

	while(app.keep_running) {
		process_event(&app);
		SDL_Delay(1);
	}
	return 0;
}

int app_init(AppState *app) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app->window = NULL;
	app->renderer = NULL;
  if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures",
				gk_window_width, gk_window_height, SDL_WINDOW_HIGH_PIXEL_DENSITY |
				SDL_WINDOW_MOUSE_CAPTURE, &app->window, &app->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
	app->w_pixels = gk_window_width *
		SDL_GetWindowPixelDensity(app->window);
	app->h_pixels = gk_window_height *
		SDL_GetWindowPixelDensity(app->window);

	// texture create with pixels and not window size -> retina display scaling
  app->window_texture = SDL_CreateTexture(
			app->renderer, SDL_PIXELFORMAT_XRGB8888,
			SDL_TEXTUREACCESS_STREAMING, 
			app->w_pixels, app->h_pixels);

	if (!app->window_texture) {
    SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
	}
	app->keep_running = true;
  app->mouse.x = 0;
  app->mouse.y = 0;
  app->mouse_left_down = 0;
  app->mouse_right_down = 0;
	app->mouse_click = false;
  app->density = SDL_GetWindowPixelDensity(app->window);
  // app->text_input = false;
  // for (int i = 0; i < TEXT_BUF_MAX; i++) {
  //   app->text[i] = 0;
  // }
  return 1;
}

void process_event(AppState *app) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      app->keep_running = false;
      break;
		case SDL_EVENT_KEY_DOWN:
			switch(event.key.key) {
				case SDLK_P:
					if (!event.key.repeat) {
						cout << "number max: " << my_max<int>(82, 11) << endl;
						// create point
					}
					break;
			}
		}
	}
}
