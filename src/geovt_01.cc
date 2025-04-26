#include <cstdint>
#include <vector>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cassert>
#include <cstring>
#include <SDL3/SDL.h>

// INFO: use default arguments for functions like create_circle(default color)

#define DEBUG_MODE

using namespace std;

static const int gk_window_width = 1920;
static const int gk_window_height = 1080;
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
void draw(AppState &app, uint32_t *buffer);

uint32_t *test(AppState &app);


// dt no draw:			0.005ms
// dt with draw:		16.5ms
int main() {
	int counter = 0;
	AppState app;
	if (!app_init(&app))
		return 1;

	uint32_t *buffer = test(app);
	while(app.keep_running) {
		// if (counter++ > 100)
		// 	app.keep_running = 0;

		// std::cout << std::fixed << std::setprecision(12) << dt_ms << std::endl;

		draw(app, buffer);
		process_event(&app);
		// SDL_Delay(1);
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

	int vsync { 3 };
	assert(SDL_GetRenderVSync(app->renderer, &vsync));

	std::cout << "vsync: " << vsync << std::endl;
	// SDL_GetRenderVSync(app->renderer, SDL_RENDERER_VSYNC_DISABLED);

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
	std::cout << "w_pixels: " << app->w_pixels << std::endl;
	std::cout << "h_pixels: " << app->h_pixels << std::endl;
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
    case SDL_EVENT_MOUSE_MOTION:
      app->mouse.x = SDL_lround(event.motion.x * app->density);
      app->mouse.y = SDL_lround(event.motion.y * app->density);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      app->mouse_left_down = event.button.down;
			// handle mouse click event
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      app->mouse_left_down = event.button.down;
      break;
		}
	}
}

uint32_t *test(AppState &app) {
	// uint32_t *buffer = new uint32_t(app.w_pixels * app.h_pixels);
	uint32_t *buffer = (uint32_t *)malloc(sizeof(uint32_t)
			* (app.w_pixels * app.h_pixels));
	for (uint32_t i = 0;	i < app.w_pixels * app.h_pixels; i++) {
		buffer[i] = 0XFFFFFF00;
	}	
	return buffer;
}
#include <arm_neon.h> // NEON intrinsics on M1/M2

void fast_fill_neon(uint32_t* dest, uint32_t value, size_t count) {
    uint32x4_t v = vdupq_n_u32(value); // Duplicate the value into a 128-bit register (4 uint32_t)

    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        vst1q_u32(dest + i, v); // store 4 pixels at once
    }
    // fill remaining
    for (; i < count; ++i) {
        dest[i] = value;
    }
}

// in the objects keyword i want to have lines or circles or other stuff
void draw(AppState &app, uint32_t *buffer) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.window_texture, NULL, &pixels, &pitch)) {

		// std::memset(pixels, 0xFF, 4 * app.w_pixels * app.h_pixels);
    // fast_fill_neon((uint32_t*)pixels, 0xFFFFFF00, app.w_pixels * app.h_pixels);

		auto in_t1 = std::chrono::high_resolution_clock::now();
		std::fill_n((uint32_t*)pixels, app.w_pixels * app.h_pixels, 0xFF00FFFF);
		auto in_t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> 
			dt_ms_in = in_t2 - in_t1;
		std::cout << "dt_ins: " << dt_ms_in << std::endl;

		// for (uint32_t i = 0;	i < app.w_pixels * app.h_pixels; i++) {
		// 	((uint32_t *)pixels)[i] = 0XFFFFFF00;
		// }	
		// std::memcpy(pixels, buffer, 4 * app.w_pixels * app.h_pixels);
		SDL_UnlockTexture(app.window_texture);
	}



	SDL_RenderTexture(app.renderer, app.window_texture, NULL, NULL);
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> 
		dt_ms = t2 - t1;
	std::cout << "dt_out: " << dt_ms << std::endl;

	SDL_RenderPresent(app.renderer);


}
