#include "core.hpp"
#include "app.hpp"
#include "graphics.hpp"
#include "rasterize.hpp"
#include "turtle.hpp"

// this will be dynamic later, not constant
constexpr const int gk_window_width = 1920/2;
constexpr int gk_window_height = 1080/2;

const double &PI = numbers::pi;

Vec2 rotate(Vec2 &v, double angle) {
	Vec2 a;
	a.x = v.x * cos(angle) - v.y * -sin(angle);
	a.y = v.x * -sin(angle) + v.y * cos(angle);
	return a;
}

void turtle_action(Turtle &turtle, char c, uint32_t *pixbuf, int width,
                   int height, int color) {
  switch (c) {
  case 'F':
    turtle::move_draw(turtle, pixbuf, width, height, color);
    break;
  case 'f':
    turtle::move(turtle);
    break;
  case '-':
    turtle::turn_left(turtle);
    break;
  case '+':
    turtle::turn_right(turtle);
    break;
  default:
    break;
  }
}

string l_system(string s, string gen, int n) {
	string l_string = "";
	if (n == 0) {
		return gen;
	} else {
		for (int i = 0; i < (int)s.size(); i++) {
			if (s[i] == 'F') {
				l_string += l_system(gen, gen, n-1);
			} else {
				l_string += s[i];
			}
		}
	}
	return l_string;
}

int main() {
	App app;
	app::init(app, gk_window_width, gk_window_height);

	Turtle turtle{100.0, 600.0, PI/2, 10.0, PI/2};
	string initiator = "F-F-F-F";
	string generator = "F-F+F+FF-F-F+F";

	string l_str = l_system(initiator, generator, 3);
	cout << "l_str: " << l_str << endl;

	uint32_t *pixbuf = nullptr;
	int pixbuf_pitch = 0;
	// auto t1 = std::chrono::high_resolution_clock::now();
	assert(SDL_LockTexture(app.video.window_texture, NULL, 
				 reinterpret_cast<void **>(&pixbuf), &pixbuf_pitch));
	std::fill_n(pixbuf, app.video.width * app.video.height, color::bg);


	for (int i = 0; i < (int)l_str.size(); i++) {
		turtle_action(turtle, l_str[i], pixbuf, app.video.width,
									app.video.height, color::fg);
	}


	SDL_UnlockTexture(app.video.window_texture);
	pixbuf = nullptr;
	pixbuf_pitch = 0;
	SDL_RenderTexture(app.video.renderer, app.video.window_texture, NULL, NULL);
	SDL_RenderPresent(app.video.renderer);
	// auto t2 = std::chrono::high_resolution_clock::now();
	// std::chrono::duration<double, std::milli> dt_ms = t2 - t1;
	// std::cout << "dt_out: " << dt_ms << std::endl;

	while(app.context.keep_running) {
		app::process_events(app);
		SDL_Delay(1);
	}
}


