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
static const uint32_t gk_conceal_color = 0x006c6c6c; //grey


// Basic Shapes
struct Vec2 {
	double x {};
	double y {};
};

enum struct ShapeState {
	NORMAL,
	HIGHLIGHTED,
	SELECTED,
	CONCEALED,
};

// How should the destructor look here?, i need to delete is_points if possible
struct Line2 {
	Vec2 p1 {};
	Vec2 p2 {};
	uint32_t id {};
	ShapeState state = ShapeState::NORMAL;
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
	uint32_t id {};
	ShapeState state = ShapeState::NORMAL;
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
  bool mouse_left_down = false;
  bool mouse_right_down = false;
	bool mouse_click = false;

	void frame_reset() { mouse_click = false; };
};

string state_to_string(ShapeState &state) {
  string s{};
  switch (state) {
  case ShapeState::NORMAL:
    s = "NORMAL";
    break;
  case ShapeState::HIGHLIGHTED:
    s = "HIGHLIGHTED";
    break;
  case ShapeState::SELECTED:
    s = "SELECTED";
    break;
  case ShapeState::CONCEALED:
    s = "CONCEALED";
    break;
  }
	return s;
}

struct IdPoint {
	Vec2 p;
	vector<uint32_t> ids;
	IdPoint() {};
	IdPoint(Vec2 &point, uint32_t id) {
		p = point;
		ids.push_back(id);
	}
};

// each shape holds id, all snap points have list of id's of shapes
struct Shapes {
	vector<Line2> lines;
	Line2 temp_line {};
	vector<Circle2> circles;
	Circle2 temp_circle {};
	uint32_t id_counter {};
	bool quantity_change = false;

	Vec2 snap_point {};
	double snap_distance = 20.0;
	bool in_snap_distance = false;

	bool line_in_construction;
	bool circle_in_construction;
	void construct(AppState &, Vec2 &point); // dependent on app mode, id++
	void clear_construction() {
		line_in_construction = false;
		circle_in_construction = false;
	}

	void frame_reset() { quantity_change = false; }

	void pop_selected(AppState &app);
	void pop_by_id(int id);
	// void id_redist(); // if id_counter >= uint16_t max redist id's, not needed

	vector<IdPoint> intersection_points;
	vector<IdPoint> shape_defining_points;
	// on object count change, recalculate snap points
	// -> recalculate all is_points and append, take all vertex points and append
};

double get_point_point_distance(Vec2 &p1, Vec2 &p2) {
  return SDL_sqrt(SDL_pow(SDL_abs(p2.x - p1.x), 2.0) +
                  SDL_pow(SDL_abs(p2.y - p1.y), 2.0));
}


int app_init(AppState &app);
void process_events(AppState &app, Shapes &shapes);
void reset_states(AppState &, Shapes &);

void create_shapes(AppState &app, Shapes &shapes);
uint32_t get_color(const ShapeState &);

void graphics(AppState &app, Shapes &shapes);
void update(AppState &app, Shapes &shapes);
void draw(AppState &app, Shapes &shapes);

auto create_line(Shapes &shapes, Vec2 &p0, Vec2 &&p1)
{
	shapes.lines.push_back(Line2{ p0, p1}); // CHECK: is this valid?
	cout << "Line Created, " << shapes.lines.size() << "lines" << endl;
	return shapes.lines.end();
}

auto create_circle(Shapes &shapes, Vec2 &center, Vec2 &circum_point)
{
	shapes.circles.push_back(Circle2{ center, circum_point});
	cout << "Circle Created, " << shapes.circles.size() << "circles" << endl;
	return shapes.circles.end();
}

int main() {
	AppState app;
	Shapes shapes;
	if (!app_init(app))
		return 1;

	while(app.keep_running) {
		shapes.frame_reset();
		app.frame_reset();
		process_events(app, shapes);

		if (shapes.in_snap_distance) {
			shapes.construct(app, shapes.snap_point);
		} else {
			shapes.construct(app, app.mouse);
		}

		graphics(app, shapes);
		draw(app, shapes);
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

void process_events(AppState &app, Shapes &shapes) {
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
						shapes.clear_construction();
          }
          break;
				case SDLK_C:
          if (!event.key.repeat) {
            app.mode = AppMode::CIRCLE;
						shapes.clear_construction();
          }
					break;
        case SDLK_L:
          if (!event.key.repeat) {
            app.mode = AppMode::LINE;
						shapes.clear_construction();
          }
          break;

				case SDLK_P:
					if (!event.key.repeat) {
						 cout << "circles:" << endl;
						 for (auto circle : shapes.circles) {
							 cout << "(" << circle.center.x << ", "
								 << circle.center.y << ")" << endl;
						 }
						 cout << endl;
						 cout << "lines:" << endl;
						 for (auto line : shapes.lines) {
							 string state {};
							 switch (line.state) {
								 case ShapeState::NORMAL: state = "NORMAL"; break;
								 case ShapeState::HIGHLIGHTED: state = "HIGHLIGHTED"; break;
								 case ShapeState::SELECTED: state = "SELECTED"; break;
								 case ShapeState::CONCEALED: state = "CONCEALED"; break;
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

void Shapes::construct(AppState &app, Vec2 &point) {
  switch (app.mode) {
  case AppMode::NORMAL:
    break;
  case AppMode::LINE:
		if (app.mouse_click) {
      if (!line_in_construction) {
				temp_line.p1 = point;
        line_in_construction = true;
			} else {
				temp_line.p2 = point;
				temp_line.id = id_counter++;
        lines.push_back(temp_line);
				quantity_change = true;
        line_in_construction = false;
			}
		} else if (line_in_construction) {
			temp_line.p2 = point;
		}
    break;
	case AppMode::CIRCLE:
		if (app.mouse_click) {
      if (!circle_in_construction) {
				temp_circle.center = point;
        circle_in_construction = true;
			} else {
				temp_circle.circum_point = point;
				temp_circle.id = id_counter++;
        circles.push_back(temp_circle);
        circle_in_construction = false;
			}
		} else if (circle_in_construction) {
			temp_circle.circum_point = point;
		}
    break;
	}
}

// test if point and its id allready in id_points vector 
// if not append or add id
void id_point_maybe_append(vector<IdPoint> &id_points, Vec2 &point,
                           uint32_t shape_id) {
  bool point_dup = false;
  for (auto &id_point : id_points) {
    if (id_point.p.x == point.x && id_point.p.y == point.y) {
      point_dup = true;
      bool id_dup = false;
      for (auto &id : id_point.ids) {
        if (shape_id == id) {
          id_dup = true;
        }
      }
      if (!id_dup) {
        id_point.ids.push_back(shape_id);
      }
    }
  }
  if (!point_dup) {
    id_points.push_back(IdPoint{point, shape_id});
  }
}


// TODO:
// snap points only appear when draw next object gets created
// circle-line, circle-circle
// only the circle marker vector to hold snap points -> i have two
// vectors at the moment that hold the same thing
void graphics(AppState &app, Shapes &shapes) {
	if (shapes.quantity_change) {
		shapes.intersection_points.clear();
		shapes.shape_defining_points.clear();

		// calculate intersection points for every object
		// line-line:
		for (int i = 0; i < shapes.lines.size(); i++) {
			Line2 &base_line = shapes.lines.at(i);
			Vec2 p1 = { base_line.p1.x, base_line.p1.y };
			Vec2 p2 = { base_line.p2.x, base_line.p2.y };
			Vec2 a = { -(p2.y - p1.y), (p2.x - p1.x) };
			for (int j = 0; j < shapes.lines.size(); j++) {
				if (i == j) { continue; }
				Line2 &compare_line = shapes.lines.at(j);
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
				if (is_point.x >= p1.x && is_point.x <= p2.x) {
					id_point_maybe_append(shapes.intersection_points, is_point, base_line.id);
				}
			}
		}

		// line-circle intersection
		// TODO: check if between segment endpoints
		//
		// Points P on line:		P = P1 + u(P2 - P1)
		//											P = P1 + uw
		// Points on circle:	x^2 + y^2 = r^2
		for (int i = 0; i < shapes.circles.size(); i++) {
			Circle2 &base_circle = shapes.circles.at(i);
			double radius = base_circle.radius();
			for (int j = 0; j < shapes.lines.size(); j++) {
				if (i == j) { continue; }
				Line2 &compare_line = shapes.lines.at(j);
				Vec2 p1 = { compare_line.p1.x, compare_line.p1.y };
				Vec2 p2 = { compare_line.p2.x, compare_line.p2.y };

				Vec2 a = compare_line.get_a();

				// calculate distance
				double distance = SDL_abs((a.x * base_circle.center.x + a.y * base_circle.center.y +
					(-a.x * p1.x - a.y * p1.y)) /
				SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));

				if (distance < radius) {
					Vec2 closest_point = compare_line.get_point_closest_point(base_circle.center);
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

					// cout << "is_p1: " << is_p1.x << ", " << is_p1.y << endl;
					// cout << "is points: " << base_circle.is_points.size() << endl;
					// cout << "is point" << endl;
					
					// TODO: linde problem only on direction
					if (is_p1.x >= p1.x && is_p1.x <= p2.x) {
						id_point_maybe_append(shapes.intersection_points, is_p1, base_circle.id);
					}

					if (is_p2.x >= p1.x && is_p2.x <= p2.x) {
						id_point_maybe_append(shapes.intersection_points, is_p2, base_circle.id);
					}
				}
			}
		}
	}

	for (int i = 0; i < shapes.circles.size(); i++) {
		Circle2 &base_circle = shapes.circles.at(i);
		for (int j = i+1; j < shapes.circles.size(); j++) {
			// if (i == j) { continue; }
			Circle2 &compare_circle = shapes.circles.at(j);
			double d =
					get_point_point_distance(base_circle.center, compare_circle.center);
			if (d < (base_circle.radius() + compare_circle.radius())) {
				double base_meet_distance =
						(SDL_pow(base_circle.radius(), 2.0) -
						 SDL_pow(compare_circle.radius(), 2.0) + SDL_pow(d, 2.0)) /
						(2 * d);
				double h = SDL_sqrt(SDL_pow(base_circle.radius(), 2.0) -
														SDL_pow(base_meet_distance, 2.0));

				// TODO: all this need to be functions 
				Vec2 v = { (compare_circle.center.x - base_circle.center.x), 
										(compare_circle.center.y - base_circle.center.y) };
				Vec2 v_normal = { v.x / SDL_sqrt(SDL_pow(v.x, 2.0) + SDL_pow(v.y, 2.0)),
					v.y / SDL_sqrt(SDL_pow(v.x, 2.0) + SDL_pow(v.y, 2.0)) };

				Vec2 a_normal = { v_normal.y, -v_normal.x };

				Vec2 meet_point = { base_circle.center.x + v_normal.x * base_meet_distance,
					base_circle.center.y + v_normal.y * base_meet_distance };

				Vec2 is_p1 = { meet_point.x + h * a_normal.x, meet_point.y + h * a_normal.y };
				Vec2 is_p2 = { meet_point.x - h * a_normal.x, meet_point.y - h * a_normal.y };
				// cout << "is_p1: " << is_p1.x << ", " << is_p1.y << endl;
			  // cout << "is_p2: " << is_p2.x << ", " << is_p2.y << endl;
				id_point_maybe_append(shapes.intersection_points, is_p1, base_circle.id);
				id_point_maybe_append(shapes.intersection_points, is_p2, base_circle.id);
			}
		}
	}
	cout << "is points: " << endl;
	for (auto &id_point : shapes.intersection_points) {
		cout << id_point.p.x << ", " << id_point.p.y << " ";
	}
	cout << endl;

	// update object status based on mouse status and distance
	for (auto &line : shapes.lines) {
		// perpendicular vector a
		Vec2 a = { -(line.p2.y - line.p1.y), (line.p2.x - line.p1.x) };
		double distance = SDL_abs((a.x * app.mouse.x + a.y * app.mouse.y +
					(-a.x * line.p1.x - a.y * line.p1.y)) /
				SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
		SDL_assert(distance <= SDL_max(app.w_pixels, app.h_pixels));
		// mouse in line area
		if (distance < 20.0 && app.mouse.x >= line.p1.x &&
				app.mouse.x <= line.p2.x) {
			// line.state = ShapeState::HIGHLIGHTED;
			if (app.mouse_click) {
				if (line.state == ShapeState::SELECTED) {
					line.state = ShapeState::HIGHLIGHTED;
				} else {
					line.state = ShapeState::SELECTED;
				}
			} else {
				if (!(line.state == ShapeState::SELECTED)) {
					line.state = ShapeState::HIGHLIGHTED;
				}
			}
		// mouse not in line area
		} else { 
			// line.state = ShapeState::NORMAL;
			if (!(line.state == ShapeState::SELECTED)) {
				line.state = ShapeState::NORMAL;
			}

		}
	}
}

uint32_t get_color(const ShapeState &state) {
	switch (state) {
		case ShapeState::NORMAL:
			return gk_fg_color;
			break;
		case ShapeState::HIGHLIGHTED:
			return gk_hl_color;
			break;
		case ShapeState::SELECTED:
			return gk_sel_color;
			break;
		case ShapeState::CONCEALED:
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

// in the shapes keyword i want to have lines or circles or other stuff
void draw(AppState &app, Shapes &shapes) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.w_pixels * app.h_pixels, gk_bg_color);

		// draw all finished shapes
		for (const auto &line : shapes.lines) {
			draw_line(app, line, pixels_locked);
		}
		for (const auto &circle : shapes.circles) {
			draw_circle(app, circle, pixels_locked);
		}


		// draw circle around all intersetion points
		for (const auto &is_point : shapes.intersection_points) {
			Vec2 rad_point = { is_point.p.x + 20, is_point.p.y };
			// draw_circle(app, Circle2 {is_point.p, rad_point}, pixels_locked); 
		}
		// draw circle around all shape points 
		for (const auto &shape_point : shapes.shape_defining_points) {
			Vec2 rad_point = { shape_point.p.x + 20, shape_point.p.y };
			// draw_circle(app, Circle2 {shape_point.p, rad_point}, pixels_locked); 
		}


		// draw circle live if close
		shapes.in_snap_distance = false;
		if (!shapes.in_snap_distance) {
			for (auto &is_point : shapes.intersection_points) {
				if (get_point_point_distance(is_point.p, app.mouse) < shapes.snap_distance) {
					shapes.snap_point = is_point.p;
					shapes.in_snap_distance = true;
				}	
			}
		}

		if (!shapes.in_snap_distance) {
			for (auto &shape_point : shapes.shape_defining_points) {
				if (get_point_point_distance(shape_point.p, app.mouse) < shapes.snap_distance) {
					shapes.snap_point = shape_point.p;
					shapes.in_snap_distance = true;
					break;
				}
			}
		}

		if (!shapes.in_snap_distance) {
			for (auto &line : shapes.lines) {
				if (line.get_distance_to_point(app.mouse) < shapes.snap_distance) {
					shapes.snap_point = line.get_point_closest_point(app.mouse);
					shapes.in_snap_distance = true;
					break;
				}
			}
		}

		// TODO: not working
		if (!shapes.in_snap_distance) {
			for (auto &circle : shapes.circles) {
				double distance = get_point_point_distance(circle.center, app.mouse);
				if (distance < circle.radius() + shapes.snap_distance &&
						distance > circle.radius() - shapes.snap_distance) {
					//normalize
					Vec2 v = {app.mouse.x - circle.center.x,
										app.mouse.y - circle.center.y};
					Vec2 v_normal = {v.x / SDL_sqrt(SDL_pow(v.x, 2.0) + SDL_pow(v.y, 2.0)),
													v.y / SDL_sqrt(SDL_pow(v.x, 2.0) + SDL_pow(v.y, 2.0))};
					shapes.snap_point = { (circle.center.x + circle.radius() * v_normal.x),
						circle.center.y + circle.radius() * v_normal.y };
					shapes.in_snap_distance = true;
					break;
				}
			}
		}

		// draw snap circle
		Vec2 rad_point = { shapes.snap_point.x + 20, shapes.snap_point.y };
		draw_circle(app, Circle2 {shapes.snap_point, rad_point}, pixels_locked); 



		// draw the temporary shape from base to cursor live if in construction
		if (shapes.line_in_construction) {
			draw_line(app, shapes.temp_line, pixels_locked);
		}
		if (shapes.circle_in_construction) {
			draw_circle(app, shapes.temp_circle, pixels_locked);
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
