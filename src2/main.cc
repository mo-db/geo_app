#include "core.hpp"
#include "app.hpp"
#include "graphics.hpp"
#include "draw.hpp"
#include "shapes.hpp"
#include "gen.hpp"
#include "serialize.hpp"

constexpr const int gk_window_width = 1920/2;
constexpr int gk_window_height = 1080/2;

int app_init(App &app);

// info
void print_node_ids_on_click(Shapes &shapes);

void mode_change_cleanup(App &app, Shapes &shapes, GenShapes &gen_shapes);
void process_events(App &app, Shapes &shapes, GenShapes &gen_shapes);

void update_nodes(Shapes &shapes);

void check_for_changes(App &app, Shapes &shapes);

void reset_frame_state(App &app) {
	app.input.mouse_click = false;
}

void print_info(App &app, Shapes &shapes) {
	std::cout << "Mouse at (" << app.input.mouse.x << "," << app.input.mouse.y << ")\n";
	cout << "node snap: " << shapes.snap.enabled_for_node_shapes << endl;

	if (shapes.lines.size() > 0) {
		auto &line = shapes.lines[0];
		Vec2 pp = line2::project_point(line.geom, app.input.mouse);
		cout << "projected point at: " << pp.x << "," << pp.y << endl;
		double seg_dist = line2::get_distance_point_to_seg(line.geom, app.input.mouse);
		double ray_dist = line2::get_distance_point_to_ray(line.geom, app.input.mouse);
		cout << "seg_dist: " << seg_dist << endl;
		cout << "ray_dist: " << ray_dist << endl;
	}
	std::cout << "Snap ID: " << shapes.snap.id << std::endl;

}

int main() {
	App app;
	Shapes shapes;
	GenShapes gen_shapes;
	if (!app_init(app)) {
		return 1;
	}
	while(app.context.keep_running) {
		reset_frame_state(app);
		shapes.snap.in_distance = shapes::update_snap(app, shapes);

		process_events(app, shapes, gen_shapes);

		// update node points
		if (shapes.recalculate) {
			update_nodes(shapes);
		}

		// update construction
		if (shapes.snap.in_distance) {
			shapes::construct(app, shapes, shapes.snap.point);
		} else {
			shapes::construct(app, shapes, app.input.mouse);
		}

		switch (app.context.mode) {
			case AppMode::NORMAL:
				if (app.input.mouse_click) {
					shapes::update_ref(app, shapes);
					print_node_ids_on_click(shapes);
				}
				break;
			case AppMode::LINE:
				break;
			case AppMode::CIRCLE:
				break;
			case AppMode::ARC:
				break;
			case AppMode::EDIT:
				shapes::update_edit(app, shapes);
				break;
			case AppMode::GEN:
				if (shapes.snap.in_distance && app.input.mouse_click) {
					gen::maybe_select(shapes, gen_shapes);
				}
				break;
		}

		draw::plot_shapes(app, shapes);
		check_for_changes(app, shapes);
		SDL_Delay(10);
	}

}

int app_init(App &app) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app.video.window = NULL;
	app.video.renderer = NULL;
  if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures",
				gk_window_width, gk_window_height, SDL_WINDOW_HIGH_PIXEL_DENSITY |
				SDL_WINDOW_MOUSE_CAPTURE, &app.video.window, &app.video.renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

	app.video.w_pixels = gk_window_width *
		SDL_GetWindowPixelDensity(app.video.window);
	app.video.h_pixels = gk_window_height *
		SDL_GetWindowPixelDensity(app.video.window);

	// texture create with pixels and not window size . retina display scaling
  app.video.window_texture = SDL_CreateTexture(
			app.video.renderer, SDL_PIXELFORMAT_XRGB8888,
			SDL_TEXTUREACCESS_STREAMING, 
			app.video.w_pixels, app.video.h_pixels);

	if (!app.video.window_texture) {
    SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
	}
  app.video.density = SDL_GetWindowPixelDensity(app.video.window);
	std::cout << "w_pixels: " << app.video.w_pixels << std::endl;
	std::cout << "h_pixels: " << app.video.h_pixels << std::endl;

  return 1;
}

// TODO move to shapes
void print_node_ids_on_click(Shapes &shapes) {
	if (shapes.snap.in_distance) {
		if (shapes.snap.shape == SnapShape::IXN_POINT) {
			Node ixn_point = shapes.ixn_points[shapes.snap.index];
			cout << "ixn_point: " << ixn_point.P.x << ", " << ixn_point.P.y << endl;
			cout << "ids: " << endl;
			for (auto &id : ixn_point.ids) {
				cout << id << ", ";
			}
			cout << endl;
		} else if (shapes.snap.shape == SnapShape::DEF_POINT) {
			Node def_point = shapes.def_points[shapes.snap.index];
			cout << "def_point: " << def_point.P.x << ", " << def_point.P.y << endl;
			cout << "ids: " << endl;
			for (auto &id : def_point.ids) {
				cout << id << ", ";
			}
			cout << endl;
		}
	}
}

void mode_change_cleanup(App &app, Shapes &shapes, GenShapes &gen_shapes) {
	// for all modes
	shapes.construct.clear();
	shapes::clear_tflags_global(shapes);
	shapes.snap.enabled_for_node_shapes = true;

	switch (app.context.mode) {
		case AppMode::NORMAL:
			break;
		case AppMode::LINE:
			break;
		case AppMode::CIRCLE:
			break;
		case AppMode::ARC:
			break;
		case AppMode::EDIT:
			shapes::clear_edit(shapes);
			break;
		case AppMode::GEN:
			gen::clear(shapes, gen_shapes);
			break;
	}
}

void process_events(App &app, Shapes &shapes, GenShapes &gen_shapes) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      app.context.keep_running = false;
      break;
		case SDL_EVENT_KEY_UP:
			switch(event.key.key) {
				case SDLK_LSHIFT:
					if (!event.key.repeat) {
						app.input.shift_set = false;
					}
					break;
				case SDLK_LCTRL:
					if (!event.key.repeat) {
						app.input.ctrl_set = false;
					}
					break;
			}
			break;
		case SDL_EVENT_KEY_DOWN:
			switch(event.key.key) {
				case SDLK_LSHIFT:
					if (!event.key.repeat) {
						app.input.shift_set = true;
					}
					break;
				case SDLK_LCTRL:
					if (!event.key.repeat) {
						app.input.ctrl_set = true;
					}
					break;
        case SDLK_ESCAPE:
          if (!event.key.repeat) {
						switch (app.context.mode) {
							case AppMode::NORMAL:
								break;
							case AppMode::LINE:
								shapes.construct.clear();
								break;
							case AppMode::CIRCLE:
								shapes.construct.clear();
								break;
							case AppMode::ARC:
								shapes.construct.clear();
								break;
							case AppMode::EDIT:
								shapes::clear_edit(shapes);
								break;
							case AppMode::GEN:
								gen::reset(shapes, gen_shapes);
								break;
						}
						// mode_change_cleanup(app, shapes, gen_shapes);
            // app.context.mode = AppMode::NORMAL;
          }
          break;
        case SDLK_K:
          if (!event.key.repeat) {
						if (app.context.mode == AppMode::ARC) {
							if (shapes.construct.arc.geom.clockwise == true) {
								shapes.construct.arc.geom.clockwise = false;
							} else {
								shapes.construct.arc.geom.clockwise = true;
							}
						}
          }
          break;
        case SDLK_H:
          if (!event.key.repeat) {
						if (app.context.mode == AppMode::LINE ||
								app.context.mode == AppMode::LINE ||
								app.context.mode == AppMode::ARC) {
							if (shapes.construct.concealed == true) {
								shapes.construct.concealed = false;
							} else {
								shapes.construct.concealed = true;
							}
						}
          }
          break;
        case SDLK_U:
          if (!event.key.repeat) {
						if (app.context.mode == AppMode::LINE ||
								app.context.mode == AppMode::LINE ||
								app.context.mode == AppMode::ARC) {
							if (shapes.snap.enabled_for_node_shapes == true) {
								shapes.snap.enabled_for_node_shapes = false;
							} else {
								shapes.snap.enabled_for_node_shapes = true;
							}
						}
          }
          break;
				case SDLK_N:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::NORMAL;
          }
					break;
				case SDLK_A:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::ARC;
          }
					break;
				case SDLK_C:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::CIRCLE;
          }
					break;
        case SDLK_L:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::LINE;
          }
          break;
				case SDLK_G:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::GEN;
          }
          break;
				case SDLK_E:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::EDIT;
          }
          break;
				case SDLK_S:
          if (!event.key.repeat) {
						std::string save_file = "save_file";
						serialize::save_appstate(shapes, save_file);
          }
          break;
				case SDLK_O:
          if (!event.key.repeat) {
						std::string save_file = "save_file";
						serialize::load_appstate(shapes, save_file);
          }
          break;
				case SDLK_BACKSPACE:
          if (!event.key.repeat) {
						shapes::pop_selected(shapes);
          }
          break;
				case SDLK_Y:
					if (!event.key.repeat) {
						std::ofstream outf{ "Sample.txt" };
						gen::calculate_relations(shapes, gen_shapes, outf);
					}
					break;
				case SDLK_P:
					if (!event.key.repeat) {
						print_info(app, shapes);
					}
					break;
			}
    case SDL_EVENT_MOUSE_MOTION:
      app.input.mouse.x = SDL_lround(event.motion.x * app.video.density);
      app.input.mouse.y = SDL_lround(event.motion.y * app.video.density);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      app.input.mouse_left_down = event.button.down;
			app.input.mouse_click = true;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      app.input.mouse_left_down = event.button.down;
      break;
		}
	}
}

// append shape-defining and ixn_points to the IdPoints vector
void update_nodes(Shapes &shapes) {
	shapes.ixn_points.clear();
	shapes.def_points.clear();
	// append line-line intersections
	for (size_t i = 0; i < shapes.lines.size(); i++) {
		Line &l1 = shapes.lines[i];
		for (size_t j = i+1; j < shapes.lines.size(); j++) {
			Line &l2 = shapes.lines[j];
			vector<Vec2> ixn_points = graphics::Line2_Line2_intersect(l1.geom, l2.geom);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (l1.pflags.concealed || l2.pflags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				cout << "l1 id: " << l1.id << endl;
				cout << "l2 id: " << l2.id << endl;
				shapes::maybe_append_node(shapes.ixn_points,
															ixn_point, l1.id, concealed);
				shapes::maybe_append_node(shapes.ixn_points,
															ixn_point, l2.id, concealed);
			}
		}
	}
	// append line-circle intersections
	for (size_t i = 0; i < shapes.circles.size(); i++) {
		Circle &c = shapes.circles.at(i);
		for (size_t j = 0; j < shapes.lines.size(); j++) {
			Line &l = shapes.lines.at(j);
			vector<Vec2> ixn_points = graphics::Line2_Circle2_intersect(l.geom, c.geom);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (c.pflags.concealed || l.pflags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, l.id, concealed);
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, c.id, concealed);
			}
		}
	}
	// append circle-circle intersections
	for (size_t i = 0; i < shapes.circles.size(); i++) {
		Circle &c1 = shapes.circles.at(i);
		for (size_t j = i+1; j < shapes.circles.size(); j++) {
			Circle &c2 = shapes.circles.at(j);
			vector<Vec2> ixn_points = graphics::Circle2_Circle2_intersect(c1.geom, c2.geom);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (c1.pflags.concealed || c2.pflags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, c1.id, concealed);
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, c2.id, concealed);
			}
		}
	}

	// append line-arc intersections
	for (size_t i = 0; i < shapes.arcs.size(); i++) {
		Arc &a = shapes.arcs.at(i);
		for (size_t j = 0; j < shapes.lines.size(); j++) {
			Line &l = shapes.lines.at(j);
			vector<Vec2> ixn_points = graphics::Arc2_Line2_intersect(a.geom, l.geom);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (a.pflags.concealed || l.pflags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, l.id, concealed);
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, a.id, concealed);
			}
		}
	}

	// append arc-circle intersections
	for (size_t i = 0; i < shapes.arcs.size(); i++) {
		Arc &a = shapes.arcs.at(i);
		for (size_t j = i+1; j < shapes.circles.size(); j++) {
			Circle &c = shapes.circles.at(j);
			vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(a.geom, c.geom);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (a.pflags.concealed || c.pflags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, a.id, concealed);
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, c.id, concealed);
			}
		}
	}

	// append arc-arc intersections
	for (size_t i = 0; i < shapes.arcs.size(); i++) {
		Arc &a1 = shapes.arcs.at(i);
		for (size_t j = i+1; j < shapes.circles.size(); j++) {
			Arc &a2 = shapes.arcs.at(j);
			vector<Vec2> ixn_points = graphics::Arc2_Arc2_intersect(a1.geom, a2.geom);
			// maybe change ixn_point status to concealed
			bool concealed = false;
			if (a1.pflags.concealed || a2.pflags.concealed) {
				concealed = true;
			}
			for (auto &ixn_point : ixn_points) {
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, a1.id, concealed);
				shapes::maybe_append_node(shapes.ixn_points, ixn_point, a2.id, concealed);
			}
		}
	}

	// append shape-defining points
	for (auto &line : shapes.lines) {
		bool concealed = false;
		if (line.pflags.concealed) {
			concealed = true;
		}
		shapes::maybe_append_node(shapes.def_points, line.geom.A, line.id, concealed);
		shapes::maybe_append_node(shapes.def_points, line.geom.B, line.id, concealed);
	}
	for (auto &circle : shapes.circles) {
		bool concealed = false;
		if (circle.pflags.concealed) {
			concealed = true;
		}
		shapes::maybe_append_node(shapes.def_points, circle.geom.C, circle.id, concealed);
		// id_point_maybe_append(app, shapes.def_points, circle.circum_point, circle.id);
	}
	for (auto &arc : shapes.arcs) {
		bool concealed = false;
		if (arc.pflags.concealed) {
			concealed = true;
		}
		shapes::maybe_append_node(shapes.def_points, arc.geom.C, arc.id, concealed);
		shapes::maybe_append_node(shapes.def_points, arc.geom.S, arc.id, concealed);
		shapes::maybe_append_node(shapes.def_points, arc.geom.E, arc.id, concealed);
	}
}

void check_for_changes(App &app, Shapes &shapes) {
	if (shapes.quantity_change) {
		shapes.recalculate = true;
	} else {
		shapes.recalculate = false;
	}
	shapes.quantity_change = false;
}
