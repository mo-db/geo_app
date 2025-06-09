#include "core.h"
#include "graphics.h"
#include "save_sys.h"

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
enum struct SnapShape { NONE, IXN_POINT, DEF_POINT, LINE, CIRCLE, ARC, };
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

	bool construct_concealed = false;

	double length_last_selected_line() const;
	double radius_last_selected_circle() const;
	void construct_line(AppState &app, Vec2 const &pt);
	void construct_circle(AppState &app, Vec2 const &pt);
	void construct_arc(AppState &app, Vec2 const &pt);

	void construct(AppState &, const Vec2 &point);
	void clear_construction() {
		in_construction = InConstruction::NONE;
		pt_set = PtSet::NONE;
	}

	// id related
	uint32_t id_counter {};
	bool quantity_change = false;
	bool recalculate = false;

	Line2 *get_line_of_id(const int id);
	Circle2 *get_circle_of_id(const int id);
	Arc2 *get_arc_of_id(const int id);

	const double snap_distance = 20.0; // why cant i do constexpr?
	bool snap_to_id_pts = true;

	Vec2 snap_point;
	int snap_id {}; // -1 if id_point
	int snap_index {};
	bool snap_is_id_point;
	SnapShape snap_shape = SnapShape::NONE;
	bool in_snap_distance = false;
	bool update_snap(AppState &app);


	bool gen_first_set = false;
	Vec2 gen_point {};

	vector<int> gen_start_point_ids {};
	Vec2 gen_start_point {};
	bool gen_start_set = false;



	// gen_map holds shape id and direction point 
	map<int, Vec2> gen_map {};

	void pop_selected(AppState &app);
	void pop_by_id(int id);
	void load_state_from_disk(); // -> void clear_shapes(); -> reset id_counter

	vector<IdPoint> ixn_points;
	vector<IdPoint> def_points;
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
	IdPoint start_point {};
};
void maybe_gen_select(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

int app_init(AppState &app);
void process_events(AppState &app, Shapes &shapes, GenShapes &gen_shapes);
void reset_states(AppState &, Shapes &);

void create_shapes(AppState &app, Shapes &shapes);
uint32_t get_color(const ShapeFlags &flags);

void id_point_maybe_append(AppState &app, vector<IdPoint> &id_points,
													 Vec2 &point, int shape_id);
void calculate_id_points(AppState &app, Shapes &shapes);

void gfx(AppState &app, Shapes &shapes);
void update(AppState &app, Shapes &shapes);
void draw(AppState &app, Shapes &shapes, GenShapes &gen_shapes);
void get_under_cursor_info(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

void mode_change_cleanup(AppState &app, Shapes &shapes, GenShapes &gen_shapes);

vector<double> gen_line_relations(AppState &app, Shapes &shapes,
                                          GenLine &line);
vector<double> gen_circle_relations(AppState &app, Shapes &shapes,
                                          GenCircle &circle);
vector<double> gen_arc_relations(AppState &app, Shapes &shapes,
                                          GenArc &arc);

bool maybe_set_snap_point(AppState &app, Shapes &shapes);

void check_for_changes(AppState &app, Shapes &shapes);

void toogle_hl_of_id_points(Shapes &shapes);

void save_appstate(const Shapes &shapes, const std::string &save_file);
void load_appstate(Shapes &shapes, const std::string &save_file);

int main() {
	AppState app;
	Shapes shapes;
	GenShapes gen_shapes;
	if (!app_init(app))
		return 1;

	while(app.keep_running) {
		app.frame_reset();
		process_events(app, shapes, gen_shapes);
		shapes.in_snap_distance = shapes.update_snap(app);

		if (shapes.in_snap_distance) {
			shapes.construct(app, shapes.snap_point);
		} else {
			shapes.construct(app, app.mouse);
		}

		if (app.mode == AppMode::GENSELECT && shapes.in_snap_distance && app.mouse_click) {
			maybe_gen_select(app, shapes, gen_shapes);
			if (shapes.snap_is_id_point && app.ctrl_set == true) {
				toogle_hl_of_id_points(shapes);
			}
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

void clear_hl(Shapes &shapes) {
	for (auto &ixn_point : shapes.ixn_points) {ixn_point.flags.highlighted = false;}
	for (auto &def_point : shapes.def_points) {def_point.flags.highlighted = false;}
	for (auto &line: shapes.lines) {line.flags.highlighted = false;}
	for (auto &circle: shapes.circles) {circle.flags.highlighted = false;}
	for (auto &arc: shapes.arcs) {arc.flags.highlighted = false;}
}

void mode_change_cleanup(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
	// for all modes
	shapes.clear_construction();
	clear_hl(shapes);

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
		if (shapes.snap_shape == SnapShape::IXN_POINT) {
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
						// extra_cleanup()
            app.mode = AppMode::NORMAL;
          }
          break;
				// mode key to toogle something when in specific app mode
        case SDLK_K:
					//NOTE this has to be reset when switching mode -> add to const clear
          if (!event.key.repeat) {
						if (app.mode == AppMode::ARC) {
							if (shapes.temp_arc.clockwise == true) {
								shapes.temp_arc.clockwise = false;
							} else {
								shapes.temp_arc.clockwise = true;
							}
						}
          }
          break;
        case SDLK_H:
					//NOTE this has to be reset when switching mode
          if (!event.key.repeat) {
						if (app.mode == AppMode::LINE || app.mode == AppMode::LINE || app.mode == AppMode::ARC) {
							if (shapes.construct_concealed == true) {
								shapes.construct_concealed = false;
							} else {
								shapes.construct_concealed = true;
							}
						}
          }
          break;
        case SDLK_U:
					//NOTE this has to be reset when switching mode
          if (!event.key.repeat) {
						if (app.mode == AppMode::LINE || app.mode == AppMode::LINE || app.mode == AppMode::ARC) {
							if (shapes.snap_to_id_pts == true) {
								shapes.snap_to_id_pts = false;
							} else {
								shapes.snap_to_id_pts = true;
							}
						}
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
				case SDLK_R:
          if (!event.key.repeat) {
						std::string save_file = "save_file";
						save_appstate(shapes, save_file);
          }
          break;
				case SDLK_T:
          if (!event.key.repeat) {
						std::string save_file = "save_file";
						load_appstate(shapes, save_file);
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

						for (auto &gen : gen_shapes.circles) {
							relations = gen_circle_relations(app, shapes, gen);
							relations_merge.insert(relations_merge.end(), relations.begin(), relations.end());
						}

						for (auto &gen: gen_shapes.lines) {
							relations = gen_line_relations(app, shapes, gen);
							relations_merge.insert(relations_merge.end(), relations.begin(), relations.end());
						}

						for (auto &gen : gen_shapes.arcs) {
							relations = gen_arc_relations(app, shapes, gen);
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

// update snap and return true if in_snap_distance
bool Shapes::update_snap(AppState &app) {
	snap_index = -1;
	snap_point = {};
	in_snap_distance = false;
	snap_is_id_point = false;
	snap_shape = SnapShape::NONE;

	if (snap_to_id_pts) {
		for (size_t index = 0; index < ixn_points.size(); index++) {
			if (get_point_point_distance(ixn_points[index].p, app.mouse) < snap_distance) {
				snap_point = ixn_points[index].p;
				snap_shape = SnapShape::IXN_POINT;
				snap_is_id_point = true;
				snap_index = index;
				return true;
			}	
		}

		for (size_t index = 0; index < def_points.size(); index++) {
			if (get_point_point_distance(def_points[index].p, app.mouse) < snap_distance) {
				snap_point = def_points[index].p;
				snap_shape = SnapShape::DEF_POINT;
				snap_is_id_point = true;
				snap_index = index;
				return true;
			}	
		}
	}

	for (size_t index = 0; index < lines.size(); index++) {
		Line2 &line = lines[index];
		if (line.get_distance_point_to_seg(app.mouse) < snap_distance) {
			Vec2 projected_point = line.project_point_to_ray(app.mouse);
			double p1_distance = get_point_point_distance(app.mouse, line.p1);
			double p2_distance = get_point_point_distance(app.mouse, line.p2);
			if (line.check_point_within_seg_bounds(projected_point)) {
				snap_point = projected_point;
				snap_shape = SnapShape::LINE;
				snap_is_id_point = false;
				snap_index = index;
				return true;
			} else if (p1_distance < snap_distance) {
				snap_point = line.p1;
				snap_shape = SnapShape::LINE;
				snap_is_id_point = false;
				snap_index = index;
				return true;
			} else if (p2_distance < snap_distance) {
				snap_point = line.p2;
				snap_shape = SnapShape::LINE;
				snap_is_id_point = false;
				snap_index = index;
				return true;
			}
		}
	}

	for (size_t index = 0; index < circles.size(); index++) {
		Circle2 &circle = circles[index];
		double distance = get_point_point_distance(circle.center, app.mouse);
		if (distance < circle.radius() + snap_distance &&
				distance > circle.radius() - snap_distance) {
			snap_point = circle.project_point(app.mouse);
			snap_shape = SnapShape::CIRCLE;
			snap_is_id_point = false;
			snap_index = index;
			return true;
		}
	}

	for (size_t index = 0; index < arcs.size(); index++) {
		Arc2 &arc = arcs[index];
		double distance = get_point_point_distance(arc.center, app.mouse);
		if (distance < arc.radius() + snap_distance &&
				distance > arc.radius() - snap_distance) {
			if (arc.angle_between_arc_points(arc.get_angle_of_point(app.mouse))) {
			snap_point = arc.project_point(app.mouse);
			snap_shape = SnapShape::ARC;
			snap_is_id_point = false;
			snap_index = index;
			return true;
			}
		}
	}

	return false;
}

Line2 *Shapes::get_line_of_id(const int id) {
	for (auto &line : lines) {
		if (id == line.id) {
			return &line;
		}
	}
	return nullptr;
}
Circle2 *Shapes::get_circle_of_id(const int id) {
	for (auto &circle : circles) {
		if (id == circle.id) {
			return &circle;
		}
	}
	return nullptr;
}
Arc2 *Shapes::get_arc_of_id(const int id) {
	for (auto &arc : arcs) {
		if (id == arc.id) {
			return &arc;
		}
	}
	return nullptr;
}

// TODO arc, refactor? needed?
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
		if (pt_set == PtSet::NONE) {
			pt_set = PtSet::FIRST;
			in_construction = InConstruction::LINE;
			temp_line.p1 = pt;
			temp_line.flags.concealed = construct_concealed;
		} else if (pt_set == PtSet::FIRST) {
			pt_set = PtSet::SECOND;
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
			temp_line.flags.concealed = construct_concealed;
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
			pt_set = PtSet::FIRST;
			in_construction = InConstruction::CIRCLE;
			temp_circle.center = pt;
			temp_circle.flags.concealed = construct_concealed;
		} else if (pt_set == PtSet::FIRST) {
			pt_set = PtSet::SECOND;
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
			temp_circle.flags.concealed = construct_concealed;
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
			temp_arc.flags.concealed = construct_concealed;
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
			// if snap to shape, calculate ixn_point and use this to snap arc
			if (in_snap_distance && !snap_is_id_point) {
				if (snap_shape == SnapShape::LINE) {
					Line2 &snap_line = lines[snap_index];
					vector<Vec2> ixn_points = graphics::Arc2_Line2_intersect(temp_arc, snap_line);
					if (ixn_points.size() > 1) {
						if (graphics::get_point_point_distance(ixn_points.at(0), app.mouse) < 
								graphics::get_point_point_distance(ixn_points.at(1), app.mouse)) {
							temp_arc.end_point = ixn_points.at(0);
						} else {
							temp_arc.end_point = ixn_points.at(1);
						}
					} else {
						temp_arc.end_point = ixn_points.back();
					}
				} else if (snap_shape == SnapShape::CIRCLE) {
					Circle2 &snap_circle = circles[snap_index];
					vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(temp_arc, snap_circle);
					if (ixn_points.size() > 1) {
						if (graphics::get_point_point_distance(ixn_points.at(0), app.mouse) < 
								graphics::get_point_point_distance(ixn_points.at(1), app.mouse)) {
							temp_arc.end_point = ixn_points.at(0);
						} else {
							temp_arc.end_point = ixn_points.at(1);
						}
					} else {
						temp_arc.end_point = ixn_points.back();
					}
				} else if (snap_shape == SnapShape::ARC) {
					Arc2 &snap_arc = arcs[snap_index];
					vector<Vec2> ixn_points = graphics::Arc2_Arc2_intersect(temp_arc, snap_arc);
					if (ixn_points.size() > 1) {
						if (graphics::get_point_point_distance(ixn_points.at(0), app.mouse) < 
								graphics::get_point_point_distance(ixn_points.at(1), app.mouse)) {
							temp_arc.end_point = ixn_points.at(0);
						} else {
							temp_arc.end_point = ixn_points.at(1);
						}
					} else {
						temp_arc.end_point = ixn_points.back();
					}
				}
			} else {
				temp_arc.end_point = temp_arc.project_point(pt);
			}

			temp_arc.start_angle = temp_arc.get_angle_of_point(temp_arc.circum_point);
			temp_arc.end_angle = temp_arc.get_angle_of_point(temp_arc.end_point);
			temp_arc.flags.concealed = construct_concealed;
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
		temp_arc.end_point = temp_arc.project_point(pt);
		Vec2 v1 = {temp_arc.circum_point.x - temp_arc.center.x,
							 temp_arc.circum_point.y - temp_arc.center.y};
		Vec2 v2 = {temp_arc.end_point.x - temp_arc.center.x,
							 temp_arc.end_point.y - temp_arc.center.y};
		double cross = v1.x * v2.y - v1.y * v2.x;
		cout << "cross: " << cross << endl;
	}
}

void Shapes::construct(AppState &app, Vec2 const &pt) {
  switch (app.mode) {
    case AppMode::NORMAL:    return;
    case AppMode::LINE:      construct_line(app, pt);   break;
    case AppMode::CIRCLE:    construct_circle(app, pt); break;
    case AppMode::ARC:       construct_arc(app, pt);    break;
		default:								 return;
  }
}

void id_point_maybe_append(AppState &app, vector<IdPoint> &id_points, Vec2 &point,
                           int shape_id, bool pt_concealed) {
  bool point_dup = false;
  for (auto &id_point : id_points) {
		if (points_equal_with_pixel_epsilon(id_point.p, point)) {
      point_dup = true;
      for (auto &id : id_point.ids) {
				if (shape_id == id) {
					return;
        }
      }
			// add id to id point, maybe change conceal status of id point
			if (id_point.flags.concealed && !pt_concealed) {
				id_point.flags.concealed = false;
			}
      id_point.ids.push_back(shape_id);
			return;
    }
  }
	if (!point_dup) {
		// if new point push back and maybe flag as concealed
		id_points.push_back(IdPoint{point, shape_id});
		if (pt_concealed) {
			id_points.back().flags.concealed = true;
		}
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
			vector<Vec2> ixn_points = graphics::Line2_Line2_intersect(l1, l2);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (l1.flags.concealed || l2.flags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points,
															ixn_point, l1.id, concealed);
				id_point_maybe_append(app, shapes.ixn_points,
															ixn_point, l2.id, concealed);
			}
		}
	}
	// append line-circle intersections
	for (int i = 0; i < shapes.circles.size(); i++) {
		Circle2 &c = shapes.circles.at(i);
		for (int j = 0; j < shapes.lines.size(); j++) {
			Line2 &l = shapes.lines.at(j);
			vector<Vec2> ixn_points = graphics::Line2_Circle2_intersect(l, c);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (c.flags.concealed || l.flags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, l.id, concealed);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c.id, concealed);
			}
		}
	}
	// append circle-circle intersections
	for (int i = 0; i < shapes.circles.size(); i++) {
		Circle2 &c1 = shapes.circles.at(i);
		for (int j = i+1; j < shapes.circles.size(); j++) {
			Circle2 &c2 = shapes.circles.at(j);
			vector<Vec2> ixn_points = graphics::Circle2_Circle2_intersect(c1, c2);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (c1.flags.concealed || c2.flags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c1.id, concealed);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c2.id, concealed);
			}
		}
	}

	// append line-arc intersections
	for (int i = 0; i < shapes.arcs.size(); i++) {
		Arc2 &a = shapes.arcs.at(i);
		for (int j = 0; j < shapes.lines.size(); j++) {
			Line2 &l = shapes.lines.at(j);
			vector<Vec2> ixn_points = graphics::Arc2_Line2_intersect(a, l);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (a.flags.concealed || l.flags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, l.id, concealed);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, a.id, concealed);
			}
		}
	}

	// append arc-circle intersections
	for (int i = 0; i < shapes.arcs.size(); i++) {
		Arc2 &a = shapes.arcs.at(i);
		for (int j = i+1; j < shapes.circles.size(); j++) {
			Circle2 &c = shapes.circles.at(j);
			vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(a, c);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (a.flags.concealed || c.flags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, a.id, concealed);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, c.id, concealed);
			}
		}
	}

	// append arc-arc intersections
	for (int i = 0; i < shapes.arcs.size(); i++) {
		Arc2 &a1 = shapes.arcs.at(i);
		for (int j = i+1; j < shapes.circles.size(); j++) {
			Arc2 &a2 = shapes.arcs.at(j);
			vector<Vec2> ixn_points = graphics::Arc2_Arc2_intersect(a1, a2);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (a1.flags.concealed || a2.flags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, a1.id, concealed);
				id_point_maybe_append(app, shapes.ixn_points, ixn_point, a2.id, concealed);
			}
		}
	}

	// append shape-defining points
	for (auto &line : shapes.lines) {
		bool concealed = false;
		if (line.flags.concealed) {
			concealed = true;
		}
		id_point_maybe_append(app, shapes.def_points, line.p1, line.id, concealed);
		id_point_maybe_append(app, shapes.def_points, line.p2, line.id, concealed);
	}
	for (auto &circle : shapes.circles) {
		bool concealed = false;
		if (circle.flags.concealed) {
			concealed = true;
		}
		id_point_maybe_append(app, shapes.def_points, circle.center, circle.id, concealed);
		// id_point_maybe_append(app, shapes.def_points, circle.circum_point, circle.id);
	}
	for (auto &arc : shapes.arcs) {
		bool concealed = false;
		if (arc.flags.concealed) {
			concealed = true;
		}
		id_point_maybe_append(app, shapes.def_points, arc.center, arc.id, concealed);
		id_point_maybe_append(app, shapes.def_points, arc.circum_point, arc.id, concealed);
		id_point_maybe_append(app, shapes.def_points, arc.end_point, arc.id, concealed);
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

// if in Gen mode and ctrl click on id point -> toogle hl
void toogle_hl_of_id_points(Shapes &shapes) {
	assert(shapes.snap_is_id_point);
	if (shapes.snap_shape == SnapShape::IXN_POINT) {
		bool &highlighted = shapes.ixn_points[shapes.snap_index].flags.highlighted;
		if (highlighted) {
			highlighted = false;
		} else {
			highlighted = true;
		}
	} else if (shapes.snap_shape == SnapShape::DEF_POINT) {
		bool &highlighted = shapes.def_points[shapes.snap_index].flags.highlighted;
		if (highlighted) {
			highlighted = false;
		} else {
			highlighted = true;
		}
	}
}

void hl_id_points_gen_select(Shapes &shapes, int shape_id) {
	for (auto &ixn_point : shapes.ixn_points) {
		if (!ixn_point.flags.concealed) {
			for (auto &id : ixn_point.ids) {
				if (id == shape_id) {
					ixn_point.flags.highlighted = true;
					cout << "point highlighted" << endl;
					break;
				}
			}
		}
	}

	for (auto &id_point : shapes.def_points) {
		if (!id_point.flags.concealed) {
			for (auto &id : id_point.ids) {
				if (id == shape_id) {
					cout << "point highlighted" << endl;
					id_point.flags.highlighted = true;
					break;
				}
			}
		}
	}
}

// TODO implement arc gen select
void maybe_gen_select(AppState &app, Shapes &shapes, GenShapes &gen_shapes) {
  if (shapes.snap_is_id_point) {
		if (shapes.snap_shape == SnapShape::IXN_POINT) {
				shapes.gen_start_point_ids = shapes.ixn_points[shapes.snap_index].ids;
		} else if (shapes.snap_shape == SnapShape::DEF_POINT) {
				shapes.gen_start_point_ids = shapes.def_points[shapes.snap_index].ids;
		}
		shapes.gen_start_point = shapes.snap_point;
		shapes.gen_start_set = true;
  } else if (!shapes.snap_is_id_point && shapes.gen_start_set) {

		if (shapes.snap_shape == SnapShape::LINE) {
			Line2 line = shapes.lines[shapes.snap_index];
			for (auto &id : shapes.gen_start_point_ids) {
				if (line.id == id) {
					line.flags.highlighted = true;
					gen_shapes.lines.push_back(
						GenLine{line, shapes.gen_start_point, shapes.snap_point});
					hl_id_points_gen_select(shapes, line.id);
					gen_shapes.start_set = false;
					break;
				}
			}
		}

		if (shapes.snap_shape == SnapShape::CIRCLE) {
			Circle2 circle = shapes.circles[shapes.snap_index];
			for (auto & id : shapes.gen_start_point_ids) {
				if (circle.id == id) {
					for (auto &gen_circle : gen_shapes.circles) {
						bool duplicate = (gen_circle.id == circle.id) ? true : false;
					}
					circle.flags.highlighted = true;
					gen_shapes.circles.push_back(
						GenCircle{circle, shapes.gen_start_point, shapes.snap_point});
					hl_id_points_gen_select(shapes, circle.id);
					gen_shapes.start_set = false;
					break;
				}
			}
		}

		if (shapes.snap_shape == SnapShape::ARC) {
			Arc2 arc = shapes.arcs[shapes.snap_index];
			for (auto & id : shapes.gen_start_point_ids) {
				if (arc.id == id) {
					arc.flags.highlighted = true;
					gen_shapes.arcs.push_back(
						GenArc{arc, shapes.gen_start_point, shapes.snap_point});
					hl_id_points_gen_select(shapes, arc.id);
					gen_shapes.start_set = false;
					break;
				}
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

// TODO: endpoint mix with intersection points, leads to problems
vector<double> gen_line_relations(AppState &app, Shapes &shapes,
                                          GenLine &gen) {
	vector<double> distances {};
	vector<double> distance_relations {};

	Vec2 A = gen.start_point;
	Vec2 B {};
	if (A.x == gen.line.p1.x && A.y == gen.line.p1.y) {
		B = gen.line.p2;
	} else {
		B = gen.line.p1;
	}

	double max_distance = (get_point_point_distance(A, B));

	std::cout << "max distance: " << max_distance << endl;
	std::cout << "start point: " << A.x << "," << A.y << endl;
	std::cout << "end point: " << B.x << "," << B.y << endl;
	std::cout << "p1 point: " << gen.line.p1.x << "," << gen.line.p1.y << endl;
	std::cout << "p2 point: " << gen.line.p2.x << "," << gen.line.p2.y << endl;
	std::cout << "dir point: " << gen.dir_point.x << "," << gen.dir_point .y << endl;

	for (auto &ixn_point : shapes.ixn_points) {
		if (ixn_point.flags.highlighted && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
										[&](const auto &id) { return id == gen.line.id; })) {
			distances.push_back(get_point_point_distance(A, ixn_point.p));
		}
	}

	cout << "distances: ";
	for (double distance : distances) {
			std::cout << distance << " ";
	}
	std::cout << std::endl;

	// push back start and end distance if not allready in 
	auto iter = find_if(distances.begin(), distances.end(), 
			[](double distance){ return fabs(distance - 0.0) < gk::epsilon; });
	if (iter == distances.end()) {
		distances.push_back(0.0);
	}
	iter = find_if(distances.begin(), distances.end(), 
			[=](double d){ return fabs(d - max_distance) < gk::epsilon; });
	if (iter != distances.end()) {
		distances.erase(iter);
	}

	// sort ascending
	sort(distances.begin(), distances.end(), [](double v1, double v2){ return v1 < v2; });

	// normalize
	for (auto &distance : distances) {
		distance /= max_distance;
	}
	return distances;
}

// TODO: function should take some point on the circle as arg
vector<double> gen_circle_relations(AppState &app, Shapes &shapes,
                                          GenCircle &gen) {
	bool direction_clockwise = false;
	vector<double> angles {};
	vector<double> angle_relations {};

	// fill all points into vector, if a point is selected put in front
	for (auto &ixn_point : shapes.ixn_points) {
		if (ixn_point.flags.highlighted && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
										[&](const auto &id) { return id == gen.circle.id; })) {
			angles.push_back(gen.circle.get_angle_of_point(ixn_point.p));
		}
	}

	// determine direction
	double start_angle = gen.circle.get_angle_of_point(gen.start_point);
	double dir_angle = gen.circle.get_angle_of_point(gen.dir_point);
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

vector<double> gen_arc_relations(AppState &app, Shapes &shapes,
                                          GenArc &gen) {
	bool direction_clockwise = false;
	vector<double> angles {};
	vector<double> angle_relations {};

	// fill all points into vector, if a point is selected put in front
	for (auto &ixn_point : shapes.ixn_points) {
		if (ixn_point.flags.highlighted && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
										[&](const auto &id) { return id == gen.arc.id; })) {
			angles.push_back(gen.arc.get_angle_of_point(ixn_point.p));
		}
	}

	// determine direction
	double start_angle = gen.arc.get_angle_of_point(gen.start_point);
	double dir_angle = gen.arc.get_angle_of_point(gen.dir_point);
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

// chatgpt generated, there is a cleaner way for sure
void draw_arc(AppState &app, uint32_t *pixel_buf, const Arc2 &arc, uint32_t color) {
    double cx = arc.center.x;
    double cy = arc.center.y;
    double r  = arc.radius();

    for (int x = round(cx - r);
         x <= round(cx + r);
         ++x)
    {
        double dx = x - cx;
        double v = r*r - dx*dx;
        if (v < 0) v = 0;
        int dy = lround(std::sqrt(v));

        // two candidate y’s
        int y_t = round(cy - dy);
        int y_b = round(cy + dy);

        // draw top point if it lies on the arc
        if (x >= 0 && x < app.w_pixels && y_t >= 0 && y_t < app.h_pixels) {
            double ang = std::atan2(cy - y_t, dx);
            if (ang < 0) ang += 2 * numbers::pi;

            if (arc.clockwise) {
                if ((arc.start_angle < arc.end_angle && (ang <= arc.start_angle || ang >= arc.end_angle)) ||
                    (arc.start_angle > arc.end_angle && (ang <= arc.start_angle && ang >= arc.end_angle))) {
                    pixel_buf[y_t * app.w_pixels + x] = color;
                }
            } else {
                if ((arc.start_angle < arc.end_angle && ang >= arc.start_angle && ang <= arc.end_angle) ||
                    (arc.start_angle > arc.end_angle && (ang >= arc.start_angle || ang <= arc.end_angle))) {
                    pixel_buf[y_t * app.w_pixels + x] = color;
                }
            }
        }

        // draw bottom point if it lies on the arc
        if (x >= 0 && x < app.w_pixels && y_b >= 0 && y_b < app.h_pixels) {
            double ang = std::atan2(cy - y_b, dx);
            if (ang < 0) ang += 2 * numbers::pi;

            if (arc.clockwise) {
                if ((arc.start_angle < arc.end_angle && (ang <= arc.start_angle || ang >= arc.end_angle)) ||
                    (arc.start_angle > arc.end_angle && (ang <= arc.start_angle && ang >= arc.end_angle))) {
                    pixel_buf[y_b * app.w_pixels + x] = color;
                }
            } else {
                if ((arc.start_angle < arc.end_angle && ang >= arc.start_angle && ang <= arc.end_angle) ||
                    (arc.start_angle > arc.end_angle && (ang >= arc.start_angle || ang <= arc.end_angle))) {
                    pixel_buf[y_b * app.w_pixels + x] = color;
                }
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

		// draw circle around snap point
		if (shapes.in_snap_distance) {
			draw_circle(app, pixels_locked, Circle2{shapes.snap_point, 20.0}, graphics::fg_color);
		}

		// draw circle around highlighted ixn_points
		for (const auto &ixn_point : shapes.ixn_points) {
			if (ixn_point.flags.highlighted) {
				draw_circle(app, pixels_locked, Circle2{ixn_point.p, 20.0},
										get_color(ixn_point.flags));
			}
		}

		// draw circle around highlighted def_points
		for (const auto &def_point : shapes.def_points) {
			if (def_point.flags.highlighted) {
				draw_circle(app, pixels_locked, Circle2{def_point.p, 20.0},
										get_color(def_point.flags));
			}
		}

		// draw edit line
		// if (shapes.line_in_construction) {
		// 	draw_line(app, pixels_locked, shapes.edit_line, graphics::edit_color);
		// }

		// [draw the temporary shape from base to cursor live if in construction]
		if (shapes.in_construction == InConstruction::LINE) {
			draw_line(app, pixels_locked, shapes.temp_line, get_color(shapes.temp_line.flags));
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

void serialize_line(const graphics::Line2 &line, ofstream &out) {
	out << line.p1.x << " " << line.p1.y << " " 
			<< line.p2.x << " " << line.p2.y << " "
			<< static_cast<int>(line.flags.concealed) << " ";
}
graphics::Line2 deserialize_line(ifstream &in) {
	Line2 line;
	int concealed_int {};
	in >> line.p1.x >> line.p1.y >> line.p2.x >> line.p2.y
		 >> concealed_int;
	line.flags.concealed = static_cast<bool>(concealed_int);
	return line;
}

void serialize_circle(const graphics::Circle2 &circle, ofstream &out) {
	out << circle.center.x << " " << circle.center.y << " "
			<< circle.circum_point.x << " " << circle.circum_point.y << " "
			<< static_cast<int>(circle.flags.concealed) << " ";
}
Circle2 deserialize_circle(ifstream &in) {
	Circle2 circle;
	int concealed_int {};
	in >> circle.center.x >> circle.center.y
		 >> circle.circum_point.x >> circle.circum_point.y >> concealed_int;
	circle.flags.concealed = static_cast<bool>(concealed_int);
	return circle;
}

void serialize_arc(const graphics::Arc2 &arc, ofstream &out) {
	out << arc.center.x << " " << arc.center.y << " "
			<< arc.circum_point.x << " " << arc.circum_point.y << " "
			<< arc.end_point.x << " " << arc.end_point.y << " "
			<< static_cast<int>(arc.flags.concealed) << " ";
}
Arc2 deserialize_arc(ifstream &in) {
	Arc2 arc;
	int concealed_int {};
	in >> arc.center.x >> arc.center.y
		 >> arc.circum_point.x >> arc.circum_point.y
		 >> arc.end_point.x >> arc.end_point.y >> concealed_int;
	arc.flags.concealed = static_cast<bool>(concealed_int);
	arc.start_angle = arc.get_angle_of_point(arc.circum_point);
	arc.end_angle = arc.get_angle_of_point(arc.end_point);
	return arc;
}

// serialize all shape vectors using key member variables
// each vector is saved as a line with the amout at first position
void save_appstate(const Shapes &shapes, const std::string &save_file) {
	std::ofstream out(save_file);
	assert(out);

	out << shapes.lines.size() << " ";
	for (auto &line : shapes.lines) {
		serialize_line(line, out);
	}
	out << endl;

	out << shapes.circles.size() << " ";
	for (auto &circle : shapes.circles) {
		serialize_circle(circle, out);
	}
	out << endl;

	out << shapes.arcs.size() << " ";
	for (auto &arc : shapes.arcs) {
		serialize_arc(arc, out);
	}
	out << endl;
}

// clear shapes and load saved ones
// the amout of shapes to load per line is determined by the start value
void load_appstate(Shapes &shapes, const std::string &save_file) {
	std::ifstream in(save_file);
	assert(in);
	shapes.quantity_change = true;

	shapes.lines.clear();
	size_t n_lines;
	in >> n_lines;
	for (size_t i = 0; i < n_lines; i++) {
	 shapes.lines.push_back(deserialize_line(in));
	}

	shapes.circles.clear();
	size_t n_circles;
	in >> n_circles;
	for (size_t i = 0; i < n_circles; i++) {
		shapes.circles.push_back(deserialize_circle(in));
	}

	shapes.arcs.clear();
	size_t n_arcs;
	in >> n_arcs;
	for (size_t i = 0; i < n_arcs; i++) {
		shapes.arcs.push_back(deserialize_arc(in));
	}
}


void check_for_changes(AppState &app, Shapes &shapes) {
	if (shapes.quantity_change) {
		shapes.recalculate = true;
	} else {
		shapes.recalculate = false;
	}
	shapes.quantity_change = false;
}
