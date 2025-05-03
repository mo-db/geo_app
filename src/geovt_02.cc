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

// xrgb colors
static const uint32_t gk_fg_color = 0x00000000; //black
static const uint32_t gk_bg_color = 0x00FFFFFF; //white
static const uint32_t gk_hl_color = 0x000000FF; //blue
static const uint32_t gk_sel_color = 0x00FF0000; //red
static const uint32_t gk_conceal_color = 0x006c6c6c;

static bool line_under_construction = false;
static bool circle_under_construction = false;

// Basic Objects
struct Vec2 {
	double x {};
	double y {};
};

enum struct ObjectState {
	NORMAL,
	HIGHLIGHTED,
	SELECTED,
	CONCEALED,
};

// How should the destructor look here?, i need to delete is_points if possible
struct Line2 {
	Vec2 p1 {};
	Vec2 p2 {};
	vector<Vec2> is_points {};
	ObjectState state = ObjectState::NORMAL;
	Line2() {}
	Line2(Vec2 p1) : p1 {p1} {}
	Line2(Vec2 p1, Vec2 p2) : p1 {p1}, p2 {p2} {}

	// get the perpendiculair vector a to the line
	Vec2 get_a() const { return Vec2 {p2.y - p1.y, -(p2.x - p1.x)}; }

	// returns the closest point on the line to some point in the plane
	Vec2 get_point_closest_point(const Vec2 &plane_point) const {
		Vec2 a = get_a();
		Vec2 line_point {};
		double k = ((p1.x * a.x + p1.y * a.y) -
								(plane_point.x * a.x + plane_point.y * a.y)) /
							 (a.x * a.x + a.y * a.y);
		line_point.x = k * a.x + plane_point.x;
		line_point.y = k * a.y + plane_point.y;
		return line_point;
	}

	// Calculate the distance of a point to the line
	double get_distance_to_point(Vec2 &plane_point) {
		Vec2 a = get_a();
		return SDL_abs((a.x * plane_point.x + a.y * plane_point.y +
										(-a.x * p1.x - a.y * p1.y)) /
									 SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
	}
};

struct Circle2 {
 	Vec2 center {};
	Vec2 circum_point {};
	vector<Vec2> is_points {};
	ObjectState state = ObjectState::NORMAL;
	Circle2() {}
	Circle2(Vec2 center) : center {center} {}
	Circle2(Vec2 center, Vec2 circum_point)
		: center {center}, circum_point {circum_point} {}
	int radius() const {
		return SDL_sqrt(SDL_pow((center.x - circum_point.x), 2.0) +
														 SDL_pow((center.y - circum_point.y), 2.0));
	}
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

struct Objects {
	vector<Line2> lines;
	Line2 temp_line; // for construction
	bool line_in_construction = false;

	vector<Circle2> circles;
	Circle2 temp_circle; // for construction
	bool circle_in_construction = false;

	// arc, temp_arc, arc_in_construction, int temp_arc points

	bool count_change = false;
	void re_init() { count_change = false; }
	void construct(AppState &app);
	void clear_construction() {
		line_in_construction = false;
		circle_in_construction = false;
	}


	// snap points are:
	// all shape defining points
	// all snap points, checked for their uniqueness

	vector<Vec2> snap_points;
	vector<Circle2> snap_markers;
};

int app_init(AppState &app);
void process_events(AppState &app, Objects &objects);
void reset_states(AppState &, Objects &);

void create_objects(AppState &app, Objects &objects);
uint32_t get_color(const ObjectState &);

void graphics(AppState &app, Objects &objects);
void update(AppState &app, Objects &objects);
void draw(AppState &app, Objects &objects);

Vec2 line_point_closest_point(const Vec2 &plane_p, const Line2 &line);

auto create_line(Objects &objects, Vec2 &p0, Vec2 &&p1)
{
	objects.lines.push_back(Line2{ p0, p1}); // CHECK: is this valid?
	cout << "Line Created, " << objects.lines.size() << "lines" << endl;
	return objects.lines.end();
}

auto create_circle(Objects &objects, Vec2 &center, Vec2 &circum_point)
{
	objects.circles.push_back(Circle2{ center, circum_point});
	cout << "Circle Created, " << objects.circles.size() << "circles" << endl;
	return objects.circles.end();
}

int main() {
	AppState app;
	Objects objects;
	if (!app_init(app))
		return 1;

	while(app.keep_running) {
		reset_states(app, objects);
		process_events(app, objects);

		// create_objects(app, objects);

		objects.re_init();
		objects.construct(app);


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
						objects.clear_construction();
          }
          break;
				case SDLK_C:
          if (!event.key.repeat) {
            app.mode = AppMode::CIRCLE;
						objects.clear_construction();
          }
					break;
        case SDLK_L:
          if (!event.key.repeat) {
            app.mode = AppMode::LINE;
						objects.clear_construction();
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
							 string state {};
							 switch (line.state) {
								 case ObjectState::NORMAL: state = "NORMAL"; break;
								 case ObjectState::HIGHLIGHTED: state = "HIGHLIGHTED"; break;
								 case ObjectState::SELECTED: state = "SELECTED"; break;
								 case ObjectState::CONCEALED: state = "CONCEALED"; break;
							 }
							 cout << "(" << line.p1.x << ", " << line.p1.y << ")"
								 << " | " << "(" << line.p2.x << ", " << line.p2.y << ")"
								 << " state: " << state << endl;
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

void reset_states(AppState &app, Objects &objects) {
	app.mouse_click = false;
	// this should somehow be automatically set for a frame where objects
	// are created or destroyed
	objects.count_change = false; // TODO: not good
}

void Objects::construct(AppState &app) {
  switch (app.mode) {
  case AppMode::NORMAL:
    break;
  case AppMode::LINE:
		if (app.mouse_click) {
      if (!line_in_construction) {
				temp_line.p1 = app.mouse;
        line_in_construction = true;
			} else {
				temp_line.p2 = app.mouse;
        lines.push_back(temp_line);
				count_change = true;
        line_in_construction = false;
			}
		} else if (line_in_construction) {
			temp_line.p2 = app.mouse;
		}
    break;
	case AppMode::CIRCLE:
		if (app.mouse_click) {
      if (!circle_in_construction) {
				temp_circle.center = app.mouse;
        circle_in_construction = true;
			} else {
				temp_circle.circum_point = app.mouse;
        circles.push_back(temp_circle);
        circle_in_construction = false;
			}
		} else if (circle_in_construction) {
			temp_circle.circum_point = app.mouse;
		}
    break;
	}
}


// Objects::construct
void create_objects(AppState &app, Objects &objects) {
  switch (app.mode) {
  case AppMode::NORMAL:
    break;
  case AppMode::LINE:
		if (app.mouse_click) {
      if (!objects.line_in_construction) {
				objects.temp_line.p1 = app.mouse;
        objects.line_in_construction = true;
			} else {
				objects.temp_line.p2 = app.mouse;
        objects.lines.push_back(objects.temp_line);
				objects.count_change = true;
        objects.line_in_construction = false;
			}
		} else if (objects.line_in_construction) {
			objects.temp_line.p2 = app.mouse;
		}
    break;
	case AppMode::CIRCLE:
		if (app.mouse_click) {
      if (!objects.circle_in_construction) {
				objects.temp_circle.center = app.mouse;
        objects.circle_in_construction = true;
			} else {
				objects.temp_circle.circum_point = app.mouse;
        objects.circles.push_back(objects.temp_circle);
        objects.circle_in_construction = false;
			}
		} else if (objects.circle_in_construction) {
			objects.temp_circle.circum_point = app.mouse;
		}
    break;
	}
}

// Calculate closest point on a line to some point in the plane
// I.		line_p = k * a + p
// II.	line_p = p1 + t * (p2 - p1)
Vec2 line_point_closest_point(const Vec2 &plane_p, const Line2 &line) {
  Vec2 a = line.get_a();
	Vec2 line_p {};
  double k = ((line.p1.x * a.x + line.p1.y * a.y) -
              (plane_p.x * a.x + plane_p.y * a.y)) /
             (a.x * a.x + a.y * a.y);
  line_p.x = k * a.x + plane_p.x;
  line_p.y = k * a.y + plane_p.y;
	return line_p;
}

// Calculate the distance of a point to the line
double distance_point_to_line(Vec2 &plane_p, Line2 &line) {
  Vec2 a = line.get_a();
  return SDL_abs((a.x * plane_p.x + a.y * plane_p.y +
                  (-a.x * line.p1.x - a.y * line.p1.y)) /
                 SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
}

// TODO:
// snap points only appear when draw next object gets created
// circle-line, circle-circle
// only the circle marker vector to hold snap points -> i have two
// vectors at the moment that hold the same thing
void graphics(AppState &app, Objects &objects) {
	if (objects.count_change) {

		// calculate intersection points for every object
		// line-line:
		for (int i = 0; i < objects.lines.size(); i++) {
			Line2 &base_line = objects.lines.at(i);
			Vec2 p1 = { base_line.p1.x, base_line.p1.y };
			Vec2 p2 = { base_line.p2.x, base_line.p2.y };
			Vec2 a = { -(p2.y - p1.y), (p2.x - p1.x) };
			for (int j = 0; j < objects.lines.size(); j++) {
				if (i == j) { continue; }
				Line2 &compare_line = objects.lines.at(j);
				Vec2 p3 = { compare_line.p1.x, compare_line.p1.y };
				Vec2 p4 = { compare_line.p2.x, compare_line.p2.y };
				Vec2 v = { (p4.x - p3.x), (p4.y - p3.y) };
				Vec2 is_point {};
				
				// calculate the intersection
				double t = (-(-a.x * p1.x - a.y * p1.y) - a.x * p3.x -a.y * p3.y) /
					(a.x * v.x + a.y * v.y);
				is_point.x = p3.x + t * v.x;
				is_point.y = p3.y + t * v.y;
				// cout << is_point.x << ", " << is_point.y << endl;


				// check if is_point is inside line endpoints
				if (is_point.x >= p3.x && is_point.x <= p4.x
						&& is_point.x >= p1.x && is_point.x <= p2.x) {
					bool duplicate = false;

					// check if intersection allready in vector
					// need epsilon here
					for (auto &is : base_line.is_points) {
						if (is.x == is_point.x && is.y == is_point.y) {
							duplicate = true;
						}
					}
					if (!duplicate) {
						base_line.is_points.push_back(is_point);
						cout << "is point" << endl;
					}
				}
			}
		}

		// TODO: this does try on circles/lines in construction -> bug
		// line-circle intersection
		// Points P on line:		P = P1 + u(P2 - P1)
		//											P = P1 + uw
		// Points on circle:	x^2 + y^2 = r^2
		for (int i = 0; i < objects.circles.size(); i++) {
			Circle2 &base_circle = objects.circles.at(i);
			base_circle.is_points.clear(); // but then i also clear circle-circle is-points
			double radius = base_circle.radius();
			for (int j = 0; j < objects.lines.size(); j++) {
				Line2 &compare_line = objects.lines.at(j);
				Vec2 p1 = { compare_line.p1.x, compare_line.p1.y };
				Vec2 p2 = { compare_line.p2.x, compare_line.p2.y };

				Vec2 a = compare_line.get_a();

				// calculate distance
				double distance = SDL_abs((a.x * base_circle.center.x + a.y * base_circle.center.y +
					(-a.x * p1.x - a.y * p1.y)) /
				SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));

				if (distance < radius) {
					Vec2 closest_point = line_point_closest_point(base_circle.center, compare_line);
					double hight = SDL_sqrt(SDL_abs(SDL_pow(radius, 2.0) - SDL_pow(distance, 2.0)));
					cout << "closest_on line: " << closest_point.x << ", " << closest_point.y << endl;
					cout << "circle center: " << base_circle.center.x << ", " << base_circle.center.y << endl;
					cout << "distance: " << distance << endl;
					cout << "hight: " << hight << endl;

					// normalize pq
					Vec2 pq = { p2.x - p1.x , p2.y - p1.y };
					Vec2 pq_normal = { pq.x / SDL_sqrt(SDL_pow(pq.x, 2.0) + SDL_pow(pq.y, 2.0)),
						pq.y / SDL_sqrt(SDL_pow(pq.x, 2.0) + SDL_pow(pq.y, 2.0)) };

					Vec2 is_p1 { closest_point.x + hight * pq_normal.x, 
						closest_point.y + hight * pq_normal.y};
					Vec2 is_p2 { closest_point.x - hight * pq_normal.x, 
						closest_point.y - hight * pq_normal.y};
					base_circle.is_points.push_back(is_p1);
					base_circle.is_points.push_back(is_p2);

					cout << "is_p1: " << is_p1.x << ", " << is_p1.y << endl;
					cout << "is points: " << base_circle.is_points.size() << endl;
				}
			}
		}

		// calculate global snapping points
		objects.snap_points.clear();
		for (auto &line : objects.lines) {
			for (auto &is_point : line.is_points) {
				objects.snap_points.push_back(is_point);
				cout << "snap point" << endl;
				for (auto & snap_point : objects.snap_points) {
					if (snap_point.x == is_point.x && snap_point.y == is_point.y) {
						; // do nothing
					} else {
						objects.snap_points.push_back(is_point);
						cout << "snap point" << endl;
					}
				}
			}
		}

		// create snap markers
		objects.snap_markers.clear();
		for (auto &snap_point : objects.snap_points) {
			objects.snap_markers.push_back(Circle2 {snap_point, {snap_point.x + 10, snap_point.y}});
		}
	}

	// update object status based on mouse status and distance
	for (auto &line : objects.lines) {
		// perpendicular vector a
		Vec2 a = { -(line.p2.y - line.p1.y), (line.p2.x - line.p1.x) };
		double distance = SDL_abs((a.x * app.mouse.x + a.y * app.mouse.y +
					(-a.x * line.p1.x - a.y * line.p1.y)) /
				SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
		SDL_assert(distance <= SDL_max(app.w_pixels, app.h_pixels));
		// mouse in line area
		if (distance < 20.0 && app.mouse.x >= line.p1.x &&
				app.mouse.x <= line.p2.x) {
			// line.state = ObjectState::HIGHLIGHTED;
			if (app.mouse_click) {
				if (line.state == ObjectState::SELECTED) {
					line.state = ObjectState::HIGHLIGHTED;
				} else {
					line.state = ObjectState::SELECTED;
				}
			} else {
				if (!(line.state == ObjectState::SELECTED)) {
					line.state = ObjectState::HIGHLIGHTED;
				}
			}
		// mouse not in line area
		} else { 
			// line.state = ObjectState::NORMAL;
			if (!(line.state == ObjectState::SELECTED)) {
				line.state = ObjectState::NORMAL;
			}

		}
	}
}

uint32_t get_color(const ObjectState &state) {
	switch (state) {
		case ObjectState::NORMAL:
			return gk_fg_color;
			break;
		case ObjectState::HIGHLIGHTED:
			return gk_hl_color;
			break;
		case ObjectState::SELECTED:
			return gk_sel_color;
			break;
		case ObjectState::CONCEALED:
			return gk_conceal_color;
			break;
	}
}

void draw_line(AppState &app, const Line2 &line, uint32_t *pixel_buf) {
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
			pixel_buf[x + SDL_lround(y) * app.w_pixels]
				= get_color(line.state);
		}
	}
}

void draw_circle(AppState &app, const Circle2 &circle, uint32_t *pixel_buf) {
	double radius = circle.radius();
	for (int x = SDL_lround(circle.center.x - radius); 
			 x < SDL_lround(circle.center.x + radius); x++) {
		double val = SDL_pow((radius), 2.0) - SDL_pow((double)(x - circle.center.x), 2.0);
		if (val < 0) {
			val = 0.0;
		}
		/* double y = SDL_sqrt(val); */
		/* printf("%f\n", y); */
		double y_offset = SDL_sqrt(val);
		int y_top = circle.center.y - SDL_lround(y_offset);
		int y_bottom = circle.center.y + SDL_lround(y_offset);

		/* printf("y_top:%d, r:%f, \n", y_top, c[c2d_cnt].radius); */
		// Draw the top and bottom points of the circle's circumference:
		if (x >= 0 && x < app.w_pixels) {
				if (y_top >= 0 && y_top < app.h_pixels)
						pixel_buf[x + y_top * app.w_pixels] = get_color(circle.state);
				if (y_bottom >= 0 && y_bottom < app.h_pixels)
						pixel_buf[x + y_bottom * app.w_pixels] = get_color(circle.state);
		}
		/* pixs[x + SDL_lround(y_offset) * appstate->w_pixels] = 0x00000000; */
	}
}

// in the objects keyword i want to have lines or circles or other stuff
void draw(AppState &app, Objects &objects) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.w_pixels * app.h_pixels, gk_bg_color);

		for (const auto &line : objects.lines) {
			draw_line(app, line, pixels_locked);
		}

		for (const auto &circle : objects.circles) {
			draw_circle(app, circle, pixels_locked);
			for (const auto &is_point : circle.is_points) {
				Vec2 rad_point = { is_point.x + 20, is_point.y };
				draw_circle(app, Circle2 {is_point, rad_point}, pixels_locked); 
			}
		}

		for (const auto &marker : objects.snap_markers) {
			draw_circle(app, marker, pixels_locked);
		}

		for (const auto &line : objects.lines) {
			if (line.state == ObjectState::HIGHLIGHTED) {
				Vec2 c_center = line_point_closest_point(app.mouse, line);
				Vec2 rad_point = { c_center.x + 20, c_center.y };
				draw_circle(app, Circle2 {c_center, rad_point}, pixels_locked); 
			}
		}

		if (objects.line_in_construction) {
			draw_line(app, objects.temp_line, pixels_locked);
		}

		if (objects.circle_in_construction) {
			draw_circle(app, objects.temp_circle, pixels_locked);
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
