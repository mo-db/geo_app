#include "include.h"
#include "graphics.h"

#define DEBUG_MODE

using namespace std;

constexpr const int gk_window_width = 1920/2;
constexpr int gk_window_height = 1080/2;
constexpr double gk_epsilon = 1e-6;

// the application
enum struct AppMode {
  NORMAL,
  LINE,
	CIRCLE,
	SELECT,
	GENSELECT,
	EDIT,
};

struct AppState {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *window_texture;
	double density;
	int w_pixels;
	int h_pixels;
	bool keep_running;
	AppMode mode = AppMode::NORMAL;

	Vec2 mouse;
  bool mouse_left_down = false;
  bool mouse_right_down = false;
	bool mouse_click = false;
	bool shift_set = false;

	void frame_reset() { 
		mouse_click = false;
	};

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

enum struct GenDirection {
	UP,			// line
	DOWN,		// line
	LEFT,		// circle
	RIGHT,	// circle
};

// each shape holds id, all snap points have list of id's of shapes
struct Shapes {
	vector<Line2> lines;
	Line2 temp_line {};
	Line2 edit_line {};
	int edit_line_point {};
	vector<Circle2> circles;
	Circle2 temp_circle {};
	uint32_t id_counter {};
	bool quantity_change = false;
	bool recalc = false;

	Vec2 snap_point {};
	double snap_distance = 20.0;
	
	// these values only make sense if in_snap_distance
	bool in_snap_distance = false;
	bool snap_is_id_point = false;
	int snap_id {};

	bool gen_first_set = false;
	Vec2 gen_point {};
	// gen_map holds shape id and direction point 
	map<int, Vec2> gen_map {};

	bool line_in_construction = false;
	bool line_in_edit = false;
	bool circle_in_construction = false;
	void construct(AppState &, Vec2 &point); // dependent on app mode, id++
	void clear_construction() {
		line_in_construction = false;
		circle_in_construction = false;
	}

	void pop_selected(AppState &app);
	void pop_by_id(int id);
	// void id_redist(); // if id_counter >= uint16_t max redist id's, not needed

	vector<IdPoint> intersection_points;
	vector<IdPoint> shape_defining_points;
	// on object count change, recalculate snap points
	// -> recalculate all is_points and append, take all vertex points and append
};

struct GenLine {
	Line2 line;
	Vec2 start_point {};
	Vec2 dir_point {};
};
struct GenCircle {
	Circle2 circle;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenShapes {
	vector<GenLine> lines;
	vector<GenCircle> circles;
	bool start_set = false;
	Vec2 start_point {};
};
void maybe_gen_select(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

int app_init(AppState &app);
void process_events(AppState &app, Shapes &shapes, GenShapes &gen_shapes);
void reset_states(AppState &, Shapes &);

void create_shapes(AppState &app, Shapes &shapes);
uint32_t get_color(const ShapeState &);

void gfx(AppState &app, Shapes &shapes);
void update(AppState &app, Shapes &shapes);
void draw(AppState &app, Shapes &shapes, GenShapes &gen_shapes);
void get_under_cursor_info(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

void mode_change_cleanup(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

// delete
vector<double> get_circle_angle_relations(AppState& app, Shapes &shapes, Circle2 &circle);

vector<double> gen_circle_relations(AppState &app, Shapes &shapes,
                                          GenCircle &circle);
vector<double> gen_line_relations(AppState &app, Shapes &shapes,
                                          GenLine &line);

bool maybe_set_snap_point(AppState &app, Shapes &shapes);

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


void check_for_changes(AppState &app, Shapes &shapes);

int main() {
	AppState app;
	Shapes shapes;
	GenShapes gen_shapes;
	if (!app_init(app))
		return 1;

	while(app.keep_running) {
		app.frame_reset();
		process_events(app, shapes, gen_shapes);

		shapes.in_snap_distance = maybe_set_snap_point(app, shapes);

		// TODO: this is bad, i check for snap distance at many places
		if (shapes.in_snap_distance) {
			shapes.construct(app, shapes.snap_point);
		} else {
			shapes.construct(app, app.mouse);
		}

		if (app.mode == AppMode::GENSELECT && shapes.in_snap_distance && app.mouse_click) {
			maybe_gen_select(app, shapes, gen_shapes);
		}

		if(app.mode == AppMode::NORMAL && app.mouse_click) {
			get_under_cursor_info(app, shapes, gen_shapes);
		}

		gfx(app, shapes);
		draw(app, shapes, gen_shapes);
		check_for_changes(app, shapes);
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

void mode_change_cleanup(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
	// for all modes
	shapes.clear_construction();

	// for gen select
	if (app.mode == AppMode::GENSELECT) {
		gen_shapes.circles.clear();
		gen_shapes.lines.clear();
	}

	if (app.mode == AppMode::EDIT) {
		shapes.lines.push_back(shapes.edit_line);
		shapes.line_in_edit = false;
		shapes.edit_line_point = 0;
	}
}

void get_under_cursor_info(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
	if (shapes.in_snap_distance) {
		if (shapes.snap_is_id_point) {
			for (auto &id_point : shapes.intersection_points) {
				if (id_point.p.x == shapes.snap_point.x &&
						id_point.p.y == shapes.snap_point.y) {
					cout << "IS Point: " << id_point.p.x << ", " << id_point.p.y << endl;
					cout << "ids: " << endl;
					for (auto &id : id_point.ids) {
						cout << id << ", ";
					}
					cout << endl;
				}
			}
			for (auto &id_point : shapes.shape_defining_points) {
				if (id_point.p.x == shapes.snap_point.x &&
						id_point.p.y == shapes.snap_point.y) {
					cout << "SD Point: " << id_point.p.x << ", " << id_point.p.y << endl;
					cout << "ids: " << endl;
					for (auto &id : id_point.ids) {
						cout << id << ", ";
					}
					cout << endl;
				}
			}
		}
	}
}

void process_events(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      app.keep_running = false;
      break;
		case SDL_EVENT_KEY_UP:
			switch(event.key.key) {
				case SDLK_LSHIFT:
					if (!event.key.repeat) {
						app.shift_set = false;
					}
					break;
			}
			break;
		case SDL_EVENT_KEY_DOWN:
			switch(event.key.key) {
				case SDLK_LSHIFT:
					if (!event.key.repeat) {
						app.shift_set = true;
					}
					break;
        case SDLK_ESCAPE:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::NORMAL;
          }
          break;
				case SDLK_C:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::CIRCLE;
          }
					break;
        case SDLK_L:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::LINE;
          }
          break;
				case SDLK_S:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::SELECT;
          }
					break;
				case SDLK_G:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::GENSELECT;
          }
          break;
				case SDLK_E:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::EDIT;
          }
          break;
				case SDLK_BACKSPACE:
          if (!event.key.repeat) {
						if (app.mode == AppMode::NORMAL) {
							shapes.lines.erase(remove_if(shapes.lines.begin(), shapes.lines.end(),
									[](const Line2 &line){ return line.state == ShapeState::SELECTED; }), shapes.lines.end());
							shapes.circles.erase(remove_if(shapes.circles.begin(), shapes.circles.end(),
									[](const Circle2 &circle){ return circle.state == ShapeState::SELECTED; }), shapes.circles.end());
							shapes.quantity_change = true;
						}
          }
          break;
				case SDLK_Y:
					if (!event.key.repeat) {
						std::ofstream outf{ "Sample.txt" };
						vector<double> relations {};
						vector<double> relations_merge {};

						for (auto &circle : gen_shapes.circles) {
							relations = gen_circle_relations(app, shapes, circle);
							relations_merge.insert(relations_merge.end(), relations.begin(), relations.end());
						}

						for (auto &line: gen_shapes.lines) {
							relations = gen_line_relations(app, shapes, line);
							relations_merge.insert(relations_merge.end(), relations.begin(), relations.end());
						}

						cout << "Relations: " << endl;
						double addup {};
						for (auto &relation : relations_merge) {
							cout << relation << ", ";
							addup += relation;
							outf << relation << " ";
						}
						cout << endl;
						cout << "addup: " << addup << endl;

					}
					break;
				case SDLK_P:
					if (!event.key.repeat) {
						cout << "MOUSE: " << endl;
						cout << app.mouse.x << "," << app.mouse.y << endl;
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

// Check for all id_points and shapes if cursor in snap_distance
// If it is, set as snap point and return ture, return false if reach end
bool maybe_set_snap_point(AppState &app, Shapes &shapes) {
	for (auto &is_point : shapes.intersection_points) {
		if (get_point_point_distance(is_point.p, app.mouse) < shapes.snap_distance) {
			shapes.snap_point = is_point.p;
			shapes.snap_is_id_point = true;
			return true;
		}	
	}

	for (auto &shape_point : shapes.shape_defining_points) {
		if (get_point_point_distance(shape_point.p, app.mouse) < shapes.snap_distance) {
			shapes.snap_point = shape_point.p;
			shapes.snap_is_id_point = true;
			return true;
		}
	}

	for (auto &line : shapes.lines) {
		if (line.get_distance_point_to_seg(app.mouse) < shapes.snap_distance) {
			if (min(get_point_point_distance(app.mouse, line.p1),
						get_point_point_distance(app.mouse, line.p1)) > shapes.snap_distance) {
				shapes.snap_point = line.project_point_to_ray(app.mouse);
				shapes.snap_is_id_point = false;
				shapes.snap_id = line.id;
				return true;
			}
		}
	}

	for (auto &circle : shapes.circles) {
		double distance = get_point_point_distance(circle.center, app.mouse);
		if (distance < circle.radius() + shapes.snap_distance &&
				distance > circle.radius() - shapes.snap_distance) {

			Vec2 v = {app.mouse.x - circle.center.x, app.mouse.y - circle.center.y};
			Vec2 v_normal = v.normalize();
			shapes.snap_point = { (circle.center.x + circle.radius() * v_normal.x),
				circle.center.y + circle.radius() * v_normal.y };

			shapes.snap_is_id_point = false;
			shapes.snap_id = circle.id;
			return true;
		}
	}

	// if not in snap distance set id to no shape
	shapes.snap_id = -1;
	return false;
}

Vec2 get_point_from_last_radius(Vec2 &point, double last_radius, Circle2 &temp_circle) {
		Vec2 v = { point.x - temp_circle.center.x, point.y - temp_circle.center.y };
		Vec2 v_normal = v.normalize();
		Vec2 circum_point = { temp_circle.center.x + v_normal.x * last_radius,
			temp_circle.center.y + v_normal.y * last_radius };
		return circum_point;
}

void mode_operations(AppState &app, Vec2 &point) {
  switch (app.mode) {
  case AppMode::NORMAL:
    break;
	case AppMode::SELECT:
		if (app.shift_set) {
		}
		break;
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
				if (app.shift_set) {
					double last_radius {};
					for (auto &circle : circles) {
						if (circle.state == ShapeState::SELECTED) {
							last_radius = circle.radius();
						}
					}
					temp_circle.circum_point = get_point_from_last_radius(point, last_radius, temp_circle);
				} else {
					temp_circle.circum_point = point;
				}

				temp_circle.id = id_counter++;
        circles.push_back(temp_circle);
				quantity_change = true;
        circle_in_construction = false;
			}
		} else if (circle_in_construction) {
			// if i wanna have this to work for last selected shape
			// i probably have to put selected shapes into a vector instead of status
			if (app.shift_set) {
				double last_radius {};
				for (auto &circle : circles) {
					if (circle.state == ShapeState::SELECTED) {
						last_radius = circle.radius();
					}
				}
				temp_circle.circum_point = get_point_from_last_radius(point, last_radius, temp_circle);
			} else {
				temp_circle.circum_point = point;
			}
		}
    break;
	}
}

bool equal_with_gk_epsilon(double x, double y) {
	return (x < y + graphics::pixel_epsilon && x > y - graphics::pixel_epsilon);
}

// test if point and its id allready in id_points vector 
// if intersection allready in id_points, not append or add id
void id_point_maybe_append(AppState &app, vector<IdPoint> &id_points, Vec2 &point,
                           uint32_t shape_id) {
  bool point_dup = false;
  for (auto &id_point : id_points) {
		if (equal_with_gk_epsilon(id_point.p.x, point.x) &&
				equal_with_gk_epsilon(id_point.p.y, point.y)) {
      point_dup = true;
      for (auto &id : id_point.ids) {
				if (shape_id == id) {
					return;
        }
      }
      id_point.ids.push_back(shape_id);
			return;
    }
  }
	if (!point_dup) {
		// TODO: i should drop error if outside screen
		if (point.x < app.w_pixels && point.y < app.h_pixels) {
			id_points.push_back(IdPoint{point, shape_id});
			cout << "ID POINT APPENDED: " << point.x << "," << point.y << endl;
		}
  }
}


// TODO:
// snap points only appear when draw next object gets created
// circle-line, circle-circle
// only the circle marker vector to hold snap points -> i have two
// vectors at the moment that hold the same thing
void gfx(AppState &app, Shapes &shapes) {
	if (shapes.recalc) {
		shapes.intersection_points.clear();
		shapes.shape_defining_points.clear();

		// [line-line intersections]
		for (int i = 0; i < shapes.lines.size(); i++) {
			Line2 &base_line = shapes.lines.at(i);
			Vec2 p1 = { base_line.p1.x, base_line.p1.y };
			Vec2 p2 = { base_line.p2.x, base_line.p2.y };
			Vec2 a = base_line.get_a();
			for (int j = i+1; j < shapes.lines.size(); j++) {
				Line2 &compare_line = shapes.lines.at(j);
				Vec2 p3 = { compare_line.p1.x, compare_line.p1.y };
				Vec2 p4 = { compare_line.p2.x, compare_line.p2.y };
				Vec2 v = { (p4.x - p3.x), (p4.y - p3.y) };
				Vec2 is_point {};
				
				// calculate the intersection TODO: add to notes how this works
				double k = (-(-a.x * p1.x - a.y * p1.y) - a.x * p3.x -a.y * p3.y) /
					(a.x * v.x + a.y * v.y);
				is_point.x = p3.x + k * v.x;
				is_point.y = p3.y + k * v.y;

				if (base_line.check_point_within_seg_bounds(is_point) &&
						compare_line.check_point_within_seg_bounds(is_point)) {
					id_point_maybe_append(app, shapes.intersection_points, is_point, base_line.id);
					id_point_maybe_append(app, shapes.intersection_points, is_point, compare_line.id);
				}
			}
		}

		// [line-circle intersections]
		// TODO: no rule for line tangent to circle
		// TODO: stop at line endpoints
		for (int i = 0; i < shapes.circles.size(); i++) {
			Circle2 &base_circle = shapes.circles.at(i);
			double radius = base_circle.radius();
			for (int j = 0; j < shapes.lines.size(); j++) {
				Line2 &compare_line = shapes.lines.at(j);
				Vec2 p1 = { compare_line.p1.x, compare_line.p1.y };
				Vec2 p2 = { compare_line.p2.x, compare_line.p2.y };
				Vec2 pq = { p2.x - p1.x , p2.y - p1.y };

				double distance = compare_line.get_distance_point_to_ray(base_circle.center);
				if (distance < radius) {
					Vec2 closest_point = compare_line.project_point_to_ray(base_circle.center);
					double hight = SDL_sqrt(SDL_fabs(SDL_pow(radius, 2.0) - SDL_pow(distance, 2.0)));
					Vec2 pq_normal = pq.normalize();

					Vec2 is_p1 { closest_point.x + hight * pq_normal.x, 
						closest_point.y + hight * pq_normal.y};
					Vec2 is_p2 { closest_point.x - hight * pq_normal.x, 
						closest_point.y - hight * pq_normal.y};

					if (compare_line.check_point_within_seg_bounds(is_p1)) {
						id_point_maybe_append(app, shapes.intersection_points, is_p1, compare_line.id);
						id_point_maybe_append(app, shapes.intersection_points, is_p1, base_circle.id);
					}
					if (compare_line.check_point_within_seg_bounds(is_p2)) {
						id_point_maybe_append(app, shapes.intersection_points, is_p2, compare_line.id);
						id_point_maybe_append(app, shapes.intersection_points, is_p2, base_circle.id);
					}
				}
			}
		}
	}

	// [circle-circle intersections]
	// TODO: breaks for circle inside circle
	// TODO: no rule for touching in one point
	for (int i = 0; i < shapes.circles.size(); i++) {
		Circle2 &base_circle = shapes.circles.at(i);
		for (int j = i+1; j < shapes.circles.size(); j++) {
			Circle2 &compare_circle = shapes.circles.at(j);
			double center_distance =
					get_point_point_distance(base_circle.center, compare_circle.center);
			// TODO !!!
			if (center_distance < (base_circle.radius() + compare_circle.radius())) {
				double base_meet_distance =
						(SDL_pow(base_circle.radius(), 2.0) -
						 SDL_pow(compare_circle.radius(), 2.0) +
						 SDL_pow(center_distance, 2.0)) /
						(2 * center_distance);
				double h = SDL_sqrt(SDL_pow(base_circle.radius(), 2.0) -
														SDL_pow(base_meet_distance, 2.0));

				// TODO: all this need to be functions 
				Vec2 v = { (compare_circle.center.x - base_circle.center.x), 
										(compare_circle.center.y - base_circle.center.y) };
				Vec2 v_normal = v.normalize();
				Vec2 a_normal = { v_normal.y, -v_normal.x };

				Vec2 meet_point = { base_circle.center.x + v_normal.x * base_meet_distance,
					base_circle.center.y + v_normal.y * base_meet_distance };

				Vec2 is_p1 = { meet_point.x + h * a_normal.x, meet_point.y + h * a_normal.y };
				Vec2 is_p2 = { meet_point.x - h * a_normal.x, meet_point.y - h * a_normal.y };

				id_point_maybe_append(app, shapes.intersection_points, is_p1, base_circle.id);
				id_point_maybe_append(app, shapes.intersection_points, is_p2, base_circle.id);
				id_point_maybe_append(app, shapes.intersection_points, is_p1, compare_circle.id);
				id_point_maybe_append(app, shapes.intersection_points, is_p2, compare_circle.id);
			}
		}
	}

	// append all shape defining points to the IdPoints
	for (auto &line : shapes.lines) {
		id_point_maybe_append(app, shapes.shape_defining_points, line.p1, line.id);
		id_point_maybe_append(app, shapes.shape_defining_points, line.p2, line.id);
	}
	for (auto &circle : shapes.circles) {
		id_point_maybe_append(app, shapes.shape_defining_points, circle.center, circle.id);
	}


	// line edit feature

	if (app.mode == AppMode::EDIT && !shapes.line_in_edit) {
		// need extra edit mode and select only one line at a time here
		for (int i = 0; i < shapes.lines.size(); i++) {
			if (shapes.lines.at(i).get_distance_point_to_ray(app.mouse) <= 20.0 && app.mouse_click) {
				shapes.edit_line = shapes.lines.at(i);
				shapes.line_in_edit = true;
				shapes.lines.erase(shapes.lines.begin() + i);
				cout << "erased" << endl;
			}
		}
	} else if (app.mode == AppMode::EDIT && shapes.line_in_edit) {
		Vec2 new_p {};
		if (shapes.in_snap_distance) {
			new_p = shapes.edit_line.project_point_to_ray(shapes.snap_point);
		} else {
			new_p = shapes.edit_line.project_point_to_ray(app.mouse);
		}

		if (get_point_point_distance(shapes.edit_line.p1, app.mouse) <
				get_point_point_distance(shapes.edit_line.p2, app.mouse)) {
			shapes.edit_line.p1 = new_p; //{new_p.x, new_p.y};
		} else {
			shapes.edit_line.p2 = new_p; //{new_p.x, new_p.y};
		}

		if (app.mouse_click) {
			shapes.lines.push_back(shapes.edit_line);
			shapes.quantity_change = true;
			shapes.line_in_edit = false;
			shapes.edit_line_point = 0;
		}
	}



	// [selection processing]
	if (app.mode == AppMode::SELECT && !app.shift_set) {
		// maybe select lines
		for (auto &line : shapes.lines) {
			Vec2 a = line.get_a();
			double distance = SDL_fabs((a.x * app.mouse.x + a.y * app.mouse.y +
						(-a.x * line.p1.x - a.y * line.p1.y)) /
					SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
			SDL_assert(distance <= SDL_max(app.w_pixels, app.h_pixels));

			if ((distance < 20.0 && app.mouse.x >= min(line.p1.x, line.p2.x) &&
					app.mouse.x <= max(line.p1.x, line.p2.x))) {

				if(app.mouse_click) {
					if (line.state == ShapeState::SELECTED) {
						line.state = ShapeState::NORMAL;
					} else {
						line.state = ShapeState::SELECTED;
					}
				}
			}
		}

		// maybe select circle
		for (auto &circle : shapes.circles) {
			double d = get_point_point_distance(app.mouse, circle.center);

			if (d < circle.radius() + 20.0 && d > circle.radius() - 20.0) {
				if(app.mouse_click) {
					if (circle.state == ShapeState::SELECTED) {
						circle.state = ShapeState::NORMAL;
					} else {
						circle.state = ShapeState::SELECTED;
					}
				}
			}
		}
		// this is only implemented for intersection points at the moment
	} else if (app.mode == AppMode::SELECT && app.shift_set) {
		for (auto & id_point : shapes.intersection_points) {
			double distance = get_point_point_distance(app.mouse, id_point.p);
			if (distance < 20.0) {
				if(app.mouse_click) {
					if (id_point.state == ShapeState::SELECTED) {
						id_point.state = ShapeState::NORMAL;
					} else {
						id_point.state = ShapeState::SELECTED;
					}
				}
			}
		}
	}
}

void maybe_gen_select(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
  if (shapes.snap_is_id_point) {
    if (!gen_shapes.start_set) {
      gen_shapes.start_point = shapes.snap_point;
      gen_shapes.start_set = true;
			cout << "id point set" << endl;
    } else {
      gen_shapes.start_set = false;
			cout << "id point unset" << endl;
    }
		//TODO: check that i stay in the same shape id
  } else if (!shapes.snap_is_id_point && gen_shapes.start_set) {
    for (auto iter = shapes.lines.begin(); iter != shapes.lines.end(); iter++) {
      if (iter->id == shapes.snap_id) {
        gen_shapes.lines.push_back(
            GenLine{*iter, gen_shapes.start_point, shapes.snap_point});
        gen_shapes.start_set = false;
				cout << "gen_shape added" << endl;
      }
    }
    for (auto iter = shapes.circles.begin(); iter != shapes.circles.end(); iter++) {
      if (iter->id == shapes.snap_id) {
        gen_shapes.circles.push_back(
            GenCircle{*iter, gen_shapes.start_point, shapes.snap_point});
        gen_shapes.start_set = false;
				cout << "gen_shape added" << endl;
      }
    }
  }
}

uint32_t get_color(const ShapeState &state) {
	switch (state) {
		case ShapeState::NORMAL:
			return graphics::fg_color;
			break;
		case ShapeState::HIGHLIGHTED:
			return graphics::hl_color;
			break;
		case ShapeState::SELECTED:
			return graphics::sel_color;
			break;
		case ShapeState::CONCEALED:
			return graphics::conceal_color;
			break;
	}
}

// TODO: maybe create custom datatype?, returns 1,2,3 or 4
int get_quad_of_point_on_circle(Vec2 &center, Vec2 &point) {
	if (point.x >= center.x && point.y <= center.y) {
		return 0;
	} else if (point.x < center.x && point.y <= center.y) {
		return 1;
	} else if (point.x <= center.x && point.y > center.y) {
		return 2;
	} else if (point.x > center.x && point.y > center.y) {
		return 3;
	} else {
		exit(EXIT_FAILURE);
	}
}

// TODO: endpoint mix with intersection points, leads to problems
vector<double> gen_line_relations(AppState &app, Shapes &shapes,
                                          GenLine &line) {
	vector<double> distances {};
	vector<double> distance_relations {};

	Vec2 A = line.start_point;
	Vec2 B = (A.x == line.line.p1.x && A.y == line.line.p1.y)
		? B = line.line.p2
		: B = line.line.p1;
	cout << "line: " << line.line.p1.x << "," << line.line.p1.y << line.line.p2.x << "," << line.line.p2.y << endl;

	double max_distance = (get_point_point_distance(A, B));

	for (auto &id_point : shapes.intersection_points) {
		if (std::any_of(id_point.ids.begin(), id_point.ids.end(),
										[&](const auto &id) { return id == line.line.id; })) {
			distances.push_back(get_point_point_distance(A, id_point.p));
		}
	}

	auto iter = find_if(distances.begin(), distances.end(), 
			[](double d){ return fabs(d - 0.0) < gk_epsilon; });
	if (iter == distances.end()) {
		distances.push_back(0.0);
	}
	iter = find_if(distances.begin(), distances.end(), 
			[=](double d){ return fabs(d - max_distance) < gk_epsilon; });
	if (iter != distances.end()) {
		distances.erase(iter);
	}

	sort(distances.begin(), distances.end(), [](double v1, double v2){ return v1 < v2; });

	cout << "distances: " << endl;
	for (auto &distance : distances) {
		cout << distance << ", ";
	}
	cout << "max distance: " << max_distance << endl;
	cout << endl;
	for (auto &distance : distances) {
		distance /= max_distance;
	}
	// for (int i = 0; i < distances.size() - 1; i++) {
	// 	// distance_relations.push_back(1 / ((distances.at(i+1) - distances.at(i)) / max_distance)); // 1/val for phasor
	// 	distance_relations.push_back((distances.at(i+1) - distances.at(i)) / max_distance);
	// }
	return distances;
}

double get_angle_circum_point(Circle2 &circle, Vec2 &point) {
  Vec2 base_point{circle.center.x + circle.radius(), circle.center.y};
  double secant_distance = get_point_point_distance(base_point, point);
  double angle = std::acos(
      (2 * SDL_pow(circle.radius(), 2.0) - SDL_pow(secant_distance, 2.0)) /
      (2 * SDL_pow(circle.radius(), 2.0)));

  if (point.y < circle.center.y) {
		return angle;
  } else if (point.y > circle.center.y) {
    return 2 * numbers::pi - angle;
  } else if (point.y == circle.center.y) {
    if (point.x >= circle.center.x) {
      return 0.0;
    } else {
      return numbers::pi;
    }
  }
}

// TODO: function should take some point on the circle as arg
vector<double> gen_circle_relations(AppState &app, Shapes &shapes,
                                          GenCircle &circle) {
	bool direction_clockwise = false;
	vector<Vec2> points;
	vector<double> angles {};
	vector<double> angle_relations {};

	// fill all points into vector, if a point is selected put in front
	for (auto &id_point : shapes.intersection_points) {
		if (std::any_of(id_point.ids.begin(), id_point.ids.end(),
										[&](const auto &id) { return id == circle.circle.id; })) {
			points.push_back(id_point.p);
		}
	}

	for (auto &point : points) {
		angles.push_back(get_angle_circum_point(circle.circle, point));
	}

	// Print the rotated vector
	std::cout << "Unsorted vector: ";
	for (double angle : angles) {
			std::cout << angle << " ";
	}
	std::cout << std::endl;

	// determine direction
	double start_angle = get_angle_circum_point(circle.circle, circle.start_point);
	double dir_angle = get_angle_circum_point(circle.circle, circle.dir_point);
	double start_angle_opposite {};
	if (start_angle > numbers::pi) {
		start_angle_opposite = start_angle - numbers::pi;
		if (dir_angle >= start_angle || dir_angle < start_angle_opposite) {
			// counter clockwise
			direction_clockwise = false;
			sort(angles.begin(), angles.end(),
				[](double a1, double a2) { return a1 < a2; });
		} else {
			// clockwise
			direction_clockwise = true;
			sort(angles.begin(), angles.end(),
				[](double a1, double a2) { return a1 > a2; });
		}
	} else {
		start_angle_opposite = start_angle + numbers::pi;
		if (dir_angle >= start_angle && dir_angle < start_angle_opposite) {
			// counter clockwise
			direction_clockwise = false;
			sort(angles.begin(), angles.end(),
				[](double a1, double a2) { return a1 < a2; });
		} else {
			// clockwise
			direction_clockwise = true;
			sort(angles.begin(), angles.end(),
				[](double a1, double a2) { return a1 > a2; });
		}
	}

	// Print the rotated vector
	std::cout << "Sorted vector: ";
	for (double angle : angles) {
			std::cout << angle << " ";
	}
	std::cout << std::endl;

	auto iter = find(angles.begin(), angles.end(), start_angle);
	rotate(angles.begin(), iter, angles.end());

	// Print the rotated vector
	std::cout << "Rotated vector: ";
	for (double angle : angles) {
			std::cout << angle << " ";
	}
	std::cout << std::endl;

	vector<double> final_angles {};
	double final_angle {};

	for (auto &angle : angles) {
		if (direction_clockwise) {
			if (angle <= angles.at(0)) {
				final_angle = angles.at(0) - angle;
			} else {
				final_angle = (2 * numbers::pi - angle) + angles.at(0);
			}
		} else {
			if (angle >= angles.at(0)) {
				final_angle = angle - angles.at(0);
			} else {
				final_angle = (2 * numbers::pi + angle) - angles.at(0);
			}
		}
		final_angle /= (2 * numbers::pi);
		final_angles.push_back(final_angle);
	}

	// double angle {};

	// for (int i = 0; i < angles.size() - 1; i++) {
	// 	double this_angle = angles.at(i);
	// 	double next_angle = angles.at(i + 1);
	// 	if (direction_clockwise) {
	// 		if (this_angle > next_angle) {
	// 			angle = ((this_angle - next_angle) / (2 * numbers::pi));
	// 		} else {
	// 			angle = (((2 * numbers::pi + this_angle) - next_angle) / (2 * numbers::pi));
	// 		}
	// 	} else {
	// 		if (this_angle < next_angle) {
	// 			angle = ((next_angle - this_angle) / (2 * numbers::pi));
	// 		} else {
	// 			angle = (((2 * numbers::pi + next_angle) - this_angle) / (2 * numbers::pi));
	// 		}
	// 	}
	// 	// angle_relations.push_back(1/angle); // 1/angle for phasor
	// 	angle_relations.push_back(angle);
	// }


	// if (direction_clockwise) {
	// 	if (angles.front() < angles.back()) {
	// 		angle = ((angles.back() - angles.front()) / (2 * numbers::pi));
	// 	} else {
	// 		angle = ((2 * numbers::pi + angles.back()) - (angles.front()) / (2 * numbers::pi));
	// 	}
	// } else {
	// 	if (angles.front() > angles.back()) {
	// 		angle = ((angles.front() - angles.back()) / (2 * numbers::pi));
	// 	} else {
	// 		angle = (((2 * numbers::pi + angles.front()) - angles.back()) / (2 * numbers::pi));
	// 	}
	// }
	// // angle_relations.push_back(1/angle); 	// 1/angle for phasor
	// angle_relations.push_back(angle);

	return final_angles;
}

vector<double> get_circle_angle_relations(AppState& app, Shapes &shapes, Circle2 &circle) {
	vector<Vec2> points {};
	vector<Vec2> pos_y {};
	vector<Vec2> neg_y {};
	vector<Vec2> angle_points{};
	vector<double> angle_relations{};

	// get all intersection points that lie on the circle
	cout << "idpoints: " << endl;
	for (auto &id_point : shapes.intersection_points) {
		if (std::any_of(id_point.ids.begin(), id_point.ids.end(),
										[&](const auto &id) { return id == circle.id; })) {
			points.push_back(id_point.p);
			cout << id_point.p.x << "," << id_point.p.y << " ";
		}
	}
	cout << endl;

	// fill vectors for upper and lower part of the circle
	for (auto &point : points) {
		if (point.y <= circle.center.y) {
			pos_y.push_back(point);
		} else {
			neg_y.push_back(point);
		}
	}

	// sort
	std::sort(pos_y.begin(), pos_y.end(), [](Vec2 &p1, Vec2 &p2){
			return p1.x > p2.x;});
	std::sort(neg_y.begin(), neg_y.end(), [](Vec2 &p1, Vec2 &p2){
			return p1.x < p2.x;});

	angle_points.insert(angle_points.end(), pos_y.begin(), pos_y.end());
	angle_points.insert(angle_points.end(), neg_y.begin(), neg_y.end());
	angle_points.push_back(angle_points.front());

	for (auto & angle_point : angle_points) {
		for (auto & angle_point2 : angle_points) {
			if (SDL_fabs(angle_point.x - angle_point2.x) < 2.0 && 
					SDL_fabs(angle_point.x - angle_point2.x) > 0.001) {
				cout << "LOW DIF: " << SDL_fabs(angle_point.x - angle_point2.x) << endl;
			}
			if (SDL_fabs(angle_point.y - angle_point2.y) < 2.0 &&
					SDL_fabs(angle_point.y - angle_point2.y) > 0.001) {
				cout << "LOW DIF: " << SDL_fabs(angle_point.y - angle_point2.y) << endl;
			}
		}
	}

	for (int i = 0; i < angle_points.size() - 1; i++) {
		Vec2 p1 = angle_points.at(i);
		Vec2 p2 = angle_points.at(i + 1);
		double d = get_point_point_distance(p1, p2);

		// kosinussatz
		double angle = std::acos((2 * SDL_pow(circle.radius(), 2.0) - SDL_pow(d, 2.0)) /
														 (2 * SDL_pow(circle.radius(), 2.0)));
		angle_relations.push_back(angle);
	}
	return angle_relations;
}

// simple version, TODO: inplement Bresenham for better performance
void draw_line(AppState &app, uint32_t *pixel_buf, const Line2 &line, uint32_t color) {
	int x1 = SDL_lround(line.p1.x);
	int y1 = SDL_lround(line.p1.y);
	int x2 = SDL_lround(line.p2.x);
	int y2 = SDL_lround(line.p2.y);
	
	double dx = x2 - x1;
	double dy = y2 - y1; // (0|0) is top left, so y increases downwards

	// Distinguish if steep or shallow slope
	if (SDL_abs(dx) > SDL_abs(dy)) {
		int x;
		double y;
		double m = dy/dx;
		int step = ((x2 > x1) ? +1 : -1);
		for (x = x1; x != x2; x += step) {
			y = m * (double)(x - x1) + (double)y1;
			if (x >= 0 && x < app.w_pixels && y >= 0 && y < app.h_pixels) {
				pixel_buf[x + SDL_lround(y) * app.w_pixels] = color;
			}
		}
	} else {
		double x;
		int y;
		double m = dx/dy;
		int step = ((y2 > y1) ? +1 : -1);
		for (y = y1; y != y2; y += step) {
			x = m * (double)(y - y1) + (double)x1;
			if (x >= 0 && x < app.w_pixels && y >= 0 && y < app.h_pixels) {
				pixel_buf[SDL_lround(x) + y * app.w_pixels] = color;
			}
		}
	}
}

// TODO: improve simple version for dy sides, TODO: Implement Bresenham
void draw_circle(AppState &app, uint32_t *pixel_buf, const Circle2 &circle, uint32_t color) {
	double radius = circle.radius();
	for (int x = SDL_lround(circle.center.x - radius); 
			 x < SDL_lround(circle.center.x + radius); x++) {
		double val = SDL_pow((radius), 2.0) - SDL_pow((double)(x - circle.center.x), 2.0);
		if (val < 0) {
			val = 0.0;
		}
		double y_offset = SDL_sqrt(val);
		int y_top = circle.center.y - SDL_lround(y_offset);
		int y_bottom = circle.center.y + SDL_lround(y_offset);

		// Draw the top and bottom points of the circle's circumference:
		if (x >= 0 && x < app.w_pixels) {
				if (y_top >= 0 && y_top < app.h_pixels)
						pixel_buf[x + y_top * app.w_pixels] = color;
				if (y_bottom >= 0 && y_bottom < app.h_pixels)
						pixel_buf[x + y_bottom * app.w_pixels] = color;
		}
	}
}

// in the shapes keyword i want to have lines or circles or other stuff
void draw(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.w_pixels * app.h_pixels, graphics::bg_color);

		// [draw all finished shapes]
		for (const auto &line : shapes.lines) {
			draw_line(app, pixels_locked, line, get_color(line.state));
		}
		for (const auto &circle : shapes.circles) {
			draw_circle(app, pixels_locked, circle, get_color(circle.state));
		}

		// [draw circle around all intersetion points]
		for (const auto &is_point : shapes.intersection_points) {
			if (is_point.state == ShapeState::SELECTED) {
				Vec2 rad_point = { is_point.p.x + 20, is_point.p.y };
				draw_circle(app, pixels_locked, Circle2 {is_point.p, rad_point}, get_color(is_point.state));
			}
		}
		// [draw circle around all shape points]
		for (const auto &shape_point : shapes.shape_defining_points) {
			Vec2 rad_point = { shape_point.p.x + 20, shape_point.p.y };
			// draw_circle(app, Circle2 {shape_point.p, rad_point}, pixels_locked); 
		}

		// [draw circle around snap point]
		if (shapes.in_snap_distance) {
			Vec2 rad_point = { shapes.snap_point.x + 20, shapes.snap_point.y };
			draw_circle(app, pixels_locked, Circle2 {shapes.snap_point, rad_point}, graphics::fg_color); 
		}

		// draw geo_selected shapes and points in specific color
		for (auto &gen_line : gen_shapes.lines) {
			Vec2 rad_point = { gen_line.start_point.x + 20, gen_line.start_point.y };
			draw_circle(app, pixels_locked, Circle2 {gen_line.start_point, rad_point}, graphics::gen_color); 
			draw_line(app, pixels_locked, gen_line.line, graphics::gen_color);
		}
		for (auto &gen_circle : gen_shapes.circles) {
			Vec2 rad_point = { gen_circle.start_point.x + 20, gen_circle.start_point.y };
			draw_circle(app, pixels_locked, Circle2 {gen_circle.start_point, rad_point}, graphics::gen_color); 
			draw_circle(app, pixels_locked, gen_circle.circle, graphics::gen_color); 
		}


		// [draw the temporary shape from base to cursor live if in construction]
		if (shapes.line_in_construction) {
			draw_line(app, pixels_locked, shapes.temp_line, get_color(shapes.temp_line.state));
		}

		// draw edit line
		if (shapes.line_in_edit) {
			draw_line(app, pixels_locked, shapes.edit_line, graphics::edit_color);
		}
		if (shapes.circle_in_construction) {
			draw_circle(app, pixels_locked, shapes.temp_circle, get_color(shapes.temp_circle.state));
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

void check_for_changes(AppState &app, Shapes &shapes) {
	if (shapes.quantity_change) {
		shapes.recalc = true;
	} else {
		shapes.recalc = false;
	}
	shapes.quantity_change = false;
}
