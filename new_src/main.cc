#include "core.h"
#include "graphics.h"

#define DEBUG_MODE

using namespace std;
using graphics::Vec2;
using graphics::IdPoint;
using graphics::Line2;
using graphics::Circle2;
using graphics::Arc2;
using graphics::ShapeFlags;

constexpr const int gk_window_width = 1920/2;
constexpr int gk_window_height = 1080/2;

// the application
enum struct AppMode {
  NORMAL,
  LINE,
	CIRCLE,
	ARC,
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
	bool ctrl_set = false;

	void frame_reset() { 
		mouse_click = false;
	};

};

enum struct GenDirection {
	UP,			// line
	DOWN,		// line
	LEFT,		// circle
	RIGHT,	// circle
};

// each shape holds id, all snap points have list of id's of shapes
enum struct InConstruction { NONE, LINE, CIRCLE, ARC, };
enum struct PtSet { NONE, FIRST, SECOND, };
struct Shapes {
	vector<Line2> lines;
	Line2 temp_line {};
	Line2 edit_line {};
	int edit_line_point {};
	bool line_in_construction = false;
	bool line_in_edit = false;

	vector<Circle2> circles;
	Circle2 temp_circle {};
	bool circle_in_construction = false;
	bool circle_in_edit = false;

	vector<Arc2> arcs;
	Arc2 temp_arc {};
	bool arc_first_set;
	bool arc_in_construction = false;
	bool arc_in_edit = false;

	InConstruction in_construction = InConstruction::NONE;
	PtSet pt_set = PtSet::NONE;

	double length_last_selected_line() const;
	double radius_last_selected_circle() const;
	void construct_line(AppState &app, Vec2 const &pt);
	void construct_circle(AppState &app, Vec2 const &pt);
	void construct_arc(AppState &app, Vec2 const &pt);

	void construct(AppState &, const Vec2 &point);
	void clear_construction() {
		in_construction = InConstruction::NONE;
		pt_set = PtSet::NONE;
		line_in_construction = false;
		circle_in_construction = false;
		arc_in_construction = false;
		arc_first_set = false;
	}

	// id related
	uint32_t id_counter {};
	bool quantity_change = false;
	bool recalculate = false;

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



	void pop_selected(AppState &app);
	void pop_by_id(int id);
	// void id_redist(); // if id_counter >= uint16_t max redist id's, not needed

	vector<IdPoint> ixn_points;
	vector<IdPoint> def_points;
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

struct GenArc {
	Arc2 arc;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenShapes {
	vector<GenLine> lines;
	vector<GenCircle> circles;
	vector<GenArc> arcs;
	bool start_set = false;
	Vec2 start_point {};
};
void maybe_gen_select(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

int app_init(AppState &app);
void process_events(AppState &app, Shapes &shapes, GenShapes &gen_shapes);
void reset_states(AppState &, Shapes &);

void create_shapes(AppState &app, Shapes &shapes);
uint32_t get_color(const ShapeFlags &flags);

void id_point_maybe_append(AppState &app, vector<IdPoint> &id_points,
													 Vec2 &point, uint32_t shape_id);
void calculate_id_points(AppState &app, Shapes &shapes);

void gfx(AppState &app, Shapes &shapes);
void update(AppState &app, Shapes &shapes);
void draw(AppState &app, Shapes &shapes, GenShapes &gen_shapes);
void get_under_cursor_info(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

void mode_change_cleanup(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

// delete

double get_angle_circum_point(Circle2 &circle, Vec2 &point);
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

		if (shapes.recalculate) {
			calculate_id_points(app, shapes);
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

// TODO move somewhere else! -> print info if key event in info mode also
void get_under_cursor_info(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
	if (shapes.in_snap_distance) {
		if (shapes.snap_is_id_point) {
			for (auto &id_point : shapes.ixn_points) {
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
			for (auto &id_point : shapes.def_points) {
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
				case SDLK_LCTRL:
					if (!event.key.repeat) {
						app.ctrl_set = false;
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
				case SDLK_LCTRL:
					if (!event.key.repeat) {
						app.ctrl_set = true;
					}
					break;
        case SDLK_ESCAPE:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::NORMAL;
          }
          break;
				case SDLK_A:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.mode = AppMode::ARC;
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
									[](const Line2 &line){ return line.flags.selected; }), shapes.lines.end());
							shapes.circles.erase(remove_if(shapes.circles.begin(), shapes.circles.end(),
									[](const Circle2 &circle){ return circle.flags.selected; }), shapes.circles.end());
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
						for (auto &line :shapes.lines) {
							if (line.flags.selected) { cout << "sel, "; }
							if (line.flags.concealed) { cout << "conccealed, "; }
							if (line.flags.highlighted) { cout << "highlighed, "; }
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
	for (auto &is_point : shapes.ixn_points) {
		if (get_point_point_distance(is_point.p, app.mouse) < shapes.snap_distance) {
			shapes.snap_point = is_point.p;
			shapes.snap_is_id_point = true;
			return true;
		}	
	}

	for (auto &shape_point : shapes.def_points) {
		if (get_point_point_distance(shape_point.p, app.mouse) < shapes.snap_distance) {
			shapes.snap_point = shape_point.p;
			shapes.snap_is_id_point = true;
			return true;
		}
	}

	for (auto &line : shapes.lines) {
		if (line.get_distance_point_to_seg(app.mouse) < shapes.snap_distance) {
			if (min(get_point_point_distance(app.mouse, line.p1),
						get_point_point_distance(app.mouse, line.p2)) > shapes.snap_distance) {
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

double Shapes::length_last_selected_line() const {
  for (auto const &line : lines) {
    if (line.flags.selected) {
			return graphics::get_point_point_distance(line.p1, line.p2);
		}
	}
  return -1.;
}

double Shapes::radius_last_selected_circle() const {
  for (auto const &circle : circles) {
    if (circle.flags.selected) {
      return circle.radius();
		}
	}
  return -1.;
}

void Shapes::construct_line(AppState &app, Vec2 const &pt) {
	if (app.mouse_click) {
		if (pt_set == PtSet::FIRST) {
			pt_set = PtSet::FIRST;
			in_construction = InConstruction::LINE;
			temp_line.p1 = pt;
			temp_line.flags.concealed = app.ctrl_set;
		} else {
			if (app.shift_set) {
				double last_length = length_last_selected_line();
				if (last_length >= 0.) {
					Vec2 v_normal = Line2{temp_line.p1, pt}.get_v().normalize();
					temp_line.p2 = {temp_line.p1.x + v_normal.x * last_length, 
													temp_line.p1.y + v_normal.y * last_length};
				} else {
					temp_line.p2 = pt;
				}
			} else {
				temp_line.p2 = pt;
			}
			temp_line.id = id_counter++;
			lines.push_back(temp_line);
			quantity_change = true;
			clear_construction();
		}
	} else if (pt_set == PtSet::FIRST) {
		temp_line.p2 = pt;
	}
}

void Shapes::construct_circle(AppState &app, Vec2 const &pt) {
	if (app.mouse_click) {
		if (pt_set == PtSet::NONE) {
			temp_circle.center = pt;
			temp_circle.flags.concealed = app.ctrl_set;
			in_construction = InConstruction::CIRCLE;
			pt_set = PtSet::FIRST;
		} else if (pt_set == PtSet::FIRST) {
			if (app.shift_set) {
				double last_radius = radius_last_selected_circle();
				if (last_radius >= 0.) {
					temp_circle.set_exact_circum_point(last_radius, pt);
				} else {
					temp_circle.circum_point = pt;
				}
			} else {
				temp_circle.circum_point = pt;
			}
			temp_circle.id = id_counter++;
			circles.push_back(temp_circle);
			quantity_change = true;
			clear_construction();
		}
	} else if (pt_set == PtSet::FIRST) {
		if (app.shift_set) {
			double last_radius = radius_last_selected_circle();
			if (last_radius >= 0.) {
				temp_circle.set_exact_circum_point(last_radius, pt);
			} else {
				temp_circle.circum_point = pt;
			}
		} else {
			temp_circle.circum_point = pt;
		}
	}
}

void Shapes::construct_arc(AppState &app, Vec2 const &pt) {
	if (app.mouse_click) {
		if (pt_set == PtSet::NONE) {
			pt_set = PtSet::FIRST;
			in_construction = InConstruction::ARC;
			temp_arc.center = pt;
			temp_arc.flags.concealed = app.ctrl_set;
		} else if (pt_set == PtSet::FIRST) {
			pt_set = PtSet::SECOND;
			if (app.shift_set) {
				double last_radius = radius_last_selected_circle();
				if (last_radius >= 0.) {
					temp_arc.set_exact_circum_point(last_radius, pt);
				} else {
					temp_arc.circum_point = pt;
				}
			} else {
				temp_arc.circum_point = pt;
			}
		} else if (pt_set == PtSet::SECOND) {
			// Vec2 v = Line2{temp_circle.center, pt}.get_v().normalize();
			// double radius = temp_arc.radius();
			// temp_arc.end_point = {v.x * radius, v.y * radius};
			temp_arc.end_point = temp_arc.project_point(pt);

			temp_arc.start_angle = temp_arc.get_angle_of_point(temp_arc.circum_point);
			temp_arc.end_angle = temp_arc.get_angle_of_point(temp_arc.end_point);
			temp_arc.id = id_counter++;
			arcs.push_back(temp_arc);
			quantity_change = true;
			clear_construction();
		}
	} else if (pt_set == PtSet::FIRST) {
		if (app.shift_set) {
			double last_radius = radius_last_selected_circle();
			if (last_radius >= 0.) {
				temp_arc.set_exact_circum_point(last_radius, pt);
			} else {
				temp_arc.circum_point = pt;
			}
		} else {
			temp_arc.circum_point = pt;
		}
	} else if (pt_set == PtSet::SECOND) {
		Vec2 v = Line2{temp_circle.center, pt}.get_v().normalize();
		double radius = temp_arc.radius();
		temp_arc.end_point = {v.x * radius, v.y * radius};
	}
}

void Shapes::construct(AppState &app, Vec2 const &pt) {
  switch (app.mode) {
    case AppMode::NORMAL:    return;
    case AppMode::LINE:      construct_line(app, pt);   break;
    case AppMode::CIRCLE:    construct_circle(app, pt); break;
    case AppMode::ARC:       construct_arc(app, pt);    break;
  }
}

// test if point and its id allready in id_points vector 
// if intersection allready in id_points, not append or add id
void id_point_maybe_append(AppState &app, vector<IdPoint> &id_points, Vec2 &point,
                           uint32_t shape_id) {
  bool point_dup = false;
  for (auto &id_point : id_points) {
		if (points_equal_with_pixel_epsilon(id_point.p, point)) {
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
		id_points.push_back(IdPoint{point, shape_id});
  }
}

// append shape-defining and ixn_points to the IdPoints vector
void calculate_id_points(AppState &app, Shapes &shapes) {
	shapes.ixn_points.clear();
	shapes.def_points.clear();
	// append line-line intersections
	for (int i = 0; i < shapes.lines.size(); i++) {
		Line2 &l1 = shapes.lines.at(i);
		for (int j = i+1; j < shapes.lines.size(); j++) {
			Line2 &l2 = shapes.lines.at(j);
			vector<Vec2> ixn_points = Line2_Line2_intersect(l1, l2);
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points,
															ixn_point, l1.id);
				id_point_maybe_append(app, shapes.ixn_points,
															ixn_point, l2.id);
			}
		}
	}
	// append line-circle intersections
	for (int i = 0; i < shapes.circles.size(); i++) {
		Circle2 &c = shapes.circles.at(i);
		for (int j = 0; j < shapes.lines.size(); j++) {
			Line2 &l = shapes.lines.at(j);
			vector<Vec2> ixn_points = graphics::Line2_Circle2_intersect(l, c);
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, l.id);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c.id);
			}
		}
	}
	// append circle-circle intersections
	for (int i = 0; i < shapes.circles.size(); i++) {
		Circle2 &c1 = shapes.circles.at(i);
		for (int j = i+1; j < shapes.circles.size(); j++) {
			Circle2 &c2 = shapes.circles.at(j);
			vector<Vec2> ixn_points = Circle2_Circle2_intersect(c1, c2);
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c1.id);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c2.id);
			}
		}
	}
	// append shape-defining points
	for (auto &line : shapes.lines) {
		id_point_maybe_append(app, shapes.def_points, line.p1, line.id);
		id_point_maybe_append(app, shapes.def_points, line.p2, line.id);
	}
	for (auto &circle : shapes.circles) {
		id_point_maybe_append(app, shapes.def_points, circle.center, circle.id);
		id_point_maybe_append(app, shapes.def_points, circle.circum_point, circle.id);
	}
	for (auto &arc : shapes.arcs) {
		id_point_maybe_append(app, shapes.def_points, arc.center, arc.id);
		id_point_maybe_append(app, shapes.def_points, arc.circum_point, arc.id);
		id_point_maybe_append(app, shapes.def_points, arc.end_point, arc.id);
	}
}

void gfx(AppState &app, Shapes &shapes) {
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
		// maybe select line
		for (auto &line : shapes.lines) {
			Vec2 a = line.get_a();
			double distance = SDL_fabs((a.x * app.mouse.x + a.y * app.mouse.y +
						(-a.x * line.p1.x - a.y * line.p1.y)) /
					SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
			SDL_assert(distance <= SDL_max(app.w_pixels, app.h_pixels));

			if ((distance < 20.0 && app.mouse.x >= min(line.p1.x, line.p2.x) &&
					app.mouse.x <= max(line.p1.x, line.p2.x))) {

				if(app.mouse_click) {
					if (line.flags.selected) {
						line.flags.selected = false;
					} else {
						line.flags.selected = true;
					}
				}
			}
		}

		// maybe select circle
		for (auto &circle : shapes.circles) {
			double d = get_point_point_distance(app.mouse, circle.center);

			if (d < circle.radius() + 20.0 && d > circle.radius() - 20.0) {
				if(app.mouse_click) {
					if (circle.flags.selected) {
						circle.flags.selected = false;
					} else {
						circle.flags.selected = true;
					}
				}
			}
		}
		// maybe selext ixn_point
	} else if (app.mode == AppMode::SELECT && app.shift_set) {
		for (auto & id_point : shapes.ixn_points) {
			double distance = get_point_point_distance(app.mouse, id_point.p);
			if (distance < 20.0) {
				if(app.mouse_click) {
					if (id_point.flags.selected) {
						id_point.flags.selected = false;
					} else {
						id_point.flags.selected = true;
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

// NOTE move this into graphics namespace, sorted after importance of color
uint32_t get_color(const ShapeFlags &flags) {
	if (flags.selected) {
		return graphics::sel_color; 
	} else if (flags.concealed) {
		return graphics::conceal_color;
	} else if (flags.highlighted) {
		return graphics::hl_color;
	} else {
		return graphics::fg_color;
	}
}

// TODO: depreciated?
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

	for (auto &id_point : shapes.ixn_points) {
		if (std::any_of(id_point.ids.begin(), id_point.ids.end(),
										[&](const auto &id) { return id == line.line.id; })) {
			distances.push_back(get_point_point_distance(A, id_point.p));
		}
	}

	auto iter = find_if(distances.begin(), distances.end(), 
			[](double d){ return fabs(d - 0.0) < gk::epsilon; });
	if (iter == distances.end()) {
		distances.push_back(0.0);
	}
	iter = find_if(distances.begin(), distances.end(), 
			[=](double d){ return fabs(d - max_distance) < gk::epsilon; });
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
	for (auto &id_point : shapes.ixn_points) {
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
	return final_angles;
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

// helper—assumes all angles normalized to [0,2π)
inline bool angle_in_range(double a, double start, double end) {
    if (end >= start) {
        return (a >= start && a <= end);
    } else {
        // wraps around 2π → 0
        return (a >= start || a <= end);
    }
}

// TODO: improve simple version for dy sides, TODO: Implement Bresenham
void draw_arc(AppState &app, uint32_t *pixel_buf, const Arc2 &arc, uint32_t color) {
    double cx = arc.center.x;
    double cy = arc.center.y;
    double r  = arc.radius();

    for (int x = SDL_lround(cx - r);
         x <= SDL_lround(cx + r);
         ++x)
    {
        double dx = x - cx;
        double v = r*r - dx*dx;
        if (v < 0) v = 0;
        int dy = SDL_lround(std::sqrt(v));

        // two candidate y’s
        int y_t = SDL_lround(cy - dy);
        int y_b = SDL_lround(cy + dy);

        // draw top point if it lies on the arc
        if (x >= 0 && x < app.w_pixels && y_t >= 0 && y_t < app.h_pixels) {
						double ang = std::atan2(cy - y_t, dx);
            if (ang < 0) ang += 2 * numbers::pi;
            if (angle_in_range(ang, arc.start_angle, arc.end_angle)) {
                pixel_buf[y_t * app.w_pixels + x] = color;
            }
        }

        // draw bottom point if on the arc
        if (x >= 0 && x < app.w_pixels && y_b >= 0 && y_b < app.h_pixels) {
            double ang = std::atan2(cy - y_b, dx);
            if (ang < 0) ang += 2 * numbers::pi;
            if (angle_in_range(ang, arc.start_angle, arc.end_angle)) {
                pixel_buf[y_b * app.w_pixels + x] = color;
            }
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
			draw_line(app, pixels_locked, line, get_color(line.flags));
		}
		for (const auto &circle : shapes.circles) {
			draw_circle(app, pixels_locked, circle, get_color(circle.flags));
		}
		for (const auto &arc : shapes.arcs) {
			draw_arc(app, pixels_locked, arc, get_color(arc.flags));
		}

		// [draw circle around all intersetion points]
		for (const auto &is_point : shapes.ixn_points) {
			if (is_point.flags.selected) {
				Vec2 rad_point = { is_point.p.x + 20, is_point.p.y };
				draw_circle(app, pixels_locked, Circle2 {is_point.p, rad_point}, get_color(is_point.flags));
			}
		}
		// [draw circle around all shape points]
		for (const auto &shape_point : shapes.def_points) {
			Vec2 rad_point = { shape_point.p.x + 20, shape_point.p.y };
			draw_circle(app, pixels_locked, Circle2 {shape_point.p, rad_point}, get_color(shape_point.flags));
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
			draw_line(app, pixels_locked, shapes.temp_line, get_color(shapes.temp_line.flags));
		}

		// draw edit line
		if (shapes.in_construction == InConstruction::LINE) {
			draw_line(app, pixels_locked, shapes.edit_line, graphics::edit_color);
		}
		if (shapes.in_construction == InConstruction::CIRCLE) {
			draw_circle(app, pixels_locked, shapes.temp_circle, get_color(shapes.temp_circle.flags));
		}
		if (shapes.in_construction == InConstruction::ARC) {
			if (shapes.arc_first_set) {
				draw_arc(app, pixels_locked, shapes.temp_arc, get_color(shapes.temp_arc.flags));
			} else {
				draw_line(app, pixels_locked, 
									Line2{shapes.temp_arc.center, shapes.temp_arc.circum_point}, 
									get_color(shapes.temp_arc.flags));
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

void check_for_changes(AppState &app, Shapes &shapes) {
	if (shapes.quantity_change) {
		shapes.recalculate = true;
	} else {
		shapes.recalculate = false;
	}
	shapes.quantity_change = false;
}
