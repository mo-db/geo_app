#include "core.hpp"
#include "app.hpp"
#include "graphics.hpp"
#include "rasterize.hpp"
#include "shapes.hpp"
#include "gen.hpp"
#include "serialize.hpp"
#include "debug.hpp"

// this will be dynamic later, not constant
constexpr const int gk_window_width = 1920/2;
constexpr int gk_window_height = 1080/2;

int app_init(App &app);
void mode_change_cleanup(App &app, Shapes &shapes, GenShapes &gen_shapes);
void process_events(App &app, Shapes &shapes, GenShapes &gen_shapes);
void update_nodes(Shapes &shapes);
void check_for_changes(App &app, Shapes &shapes);
void reset_frame_state(App &app) {
	app.input.mouse_click = false;
}
uint32_t get_color(const Shapes& shapes, const Shape &shape);
void draw(App &app, Shapes &shapes);

int main() {
	App app;
	Shapes shapes;
	GenShapes gen_shapes;
	if (!app_init(app)) {
		return 1;
	}
	while(app.context.keep_running) {
		reset_frame_state(app);
		shapes::update_snap(app, shapes);

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
					if (app.input.ctrl_set) {
						shapes::maybe_select_ref(app, shapes);
					} else if (app.input.shift_set) {
						shapes::toggle_select(app, shapes);
					}
					shapes::print_node_ids(shapes);
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
			case AppMode::UNITY:
				if (shapes.snap.in_distance && app.input.mouse_click) {
					if (app.input.shift_set) {
						shapes::toggle_select(app, shapes);
					}
					if (app.input.ctrl_set) {
						shapes::maybe_select_unity(app, shapes);
					}
				}
				break;
		}

		draw(app, shapes);
		check_for_changes(app, shapes);
		SDL_Delay(2);
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

void mode_change_cleanup(App &app, Shapes &shapes, GenShapes &gen_shapes) {
	// for all modes
	shapes::clear_tflags_global(shapes);
	shapes.snap.enabled_for_node_shapes = true;

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
								shapes::clear_tflags_global(shapes);
								shapes.ref.shape = RefShape::NONE;
								shapes.ref.id = -1;
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
							case AppMode::UNITY:
								shapes::clear_tflags_global(shapes);
								shapes.unity.shape = UnityShape::NONE;
								shapes.unity.id = -1;
								break;
						}
          }
          break;
        case SDLK_K:
					// NOTE this is bad here
          if (!event.key.repeat) {
						util::toggle_bool(shapes.construct.arc.geom.clockwise);
          }
          break;
        case SDLK_H:
					// NOTE this is bad here
          if (!event.key.repeat) {
						util::toggle_bool(shapes.construct.concealed);
          }
          break;
        case SDLK_U:
					// NOTE this is bad here
          if (!event.key.repeat) {
						util::toggle_bool(shapes.snap.enabled_for_node_shapes);
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

					// TODO new stuff
				case SDLK_J:
          if (!event.key.repeat) {
						mode_change_cleanup(app, shapes, gen_shapes);
            app.context.mode = AppMode::UNITY;
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
					// TODO new stuff
				case SDLK_M:
					if (!event.key.repeat) {
						std::ofstream outf{ "Sample.txt" };
						gen::calculate_len_ratios(shapes, outf);
					}
					break;
				case SDLK_P:
					if (!event.key.repeat) {
						debug::info_print(app, shapes);
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
		Line &l1 = shapes.get_line_by_index(i);
		for (size_t j = i+1; j < shapes.lines.size(); j++) {
			Line &l2 = shapes.get_line_by_index(j);
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
		Circle &c = shapes.get_circle_by_index(i);
		for (size_t j = 0; j < shapes.lines.size(); j++) {
			Line &l = shapes.get_line_by_index(j);
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
		Circle &c1 = shapes.get_circle_by_index(i);
		for (size_t j = i+1; j < shapes.circles.size(); j++) {
			Circle &c2 = shapes.get_circle_by_index(j);
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
		Arc &a = shapes.get_arc_by_index(i);
		for (size_t j = 0; j < shapes.lines.size(); j++) {
			Line &l = shapes.get_line_by_index(j);
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
		Arc &a = shapes.get_arc_by_index(i);
		for (size_t j = i+1; j < shapes.circles.size(); j++) {
			Circle &c = shapes.get_circle_by_index(j);
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
		Arc &a1 = shapes.get_arc_by_index(i);
		for (size_t j = i+1; j < shapes.arcs.size(); j++) {
			Arc &a2 = shapes.get_arc_by_index(j);
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

uint32_t get_color(const Shapes& shapes, const Shape &shape) {
	if (shapes.ref.shape != RefShape::NONE && shape.id == shapes.ref.id) {
		return color::special;
	}

	if (shapes.unity.shape != UnityShape::NONE && shape.id == shapes.unity.id) {
		return color::hl_secondary;
	}

	// return color hirachical
	if (shape.tflags.selected) {
		return color::select; 
	} else if (shape.tflags.hl_primary) {
		return color::hl_primary;
	} else if (shape.tflags.hl_secondary) {
		return color::hl_secondary;
	} else if (shape.tflags.hl_tertiary) {
		return color::hl_tertiary;
	} else if (shape.pflags.concealed) {
		return color::conceal;
	} else {
		return color::fg;
	}
}

void draw(App &app, Shapes &shapes) {
  void *pixels;
  int pitch;
  int w = app.video.w_pixels;
  int h = app.video.h_pixels;

  if (SDL_LockTexture(app.video.window_texture, NULL, &pixels, &pitch)) {
    uint32_t *pixels_locked = (uint32_t *)pixels;
    std::fill_n((uint32_t *)pixels, app.video.w_pixels * app.video.h_pixels,
                color::bg);

    // [draw all finished shapes]
    for (const auto &line : shapes.lines) {
      rasterize::line(pixels_locked, w, h, line.geom, get_color(shapes, line));
    }
    for (const auto &circle : shapes.circles) {
      rasterize::circle(pixels_locked, w, h, circle.geom,
                        get_color(shapes, circle));
    }
    for (const auto &arc : shapes.arcs) {
      rasterize::arc(pixels_locked, w, h, arc.geom, get_color(shapes, arc));
    }

    // draw circle around snap point
    if (shapes.snap.shape != SnapShape::NONE) {
      rasterize::circle(pixels_locked, w, h,
                        Circle2{shapes.snap.point, shapes.snap.distance},
                        color::fg);
    }

    // draw circle around hl_secondary ixn_points
    for (const auto &ixn_point : shapes.ixn_points) {
      if (ixn_point.tflags.hl_secondary) {
        rasterize::circle(pixels_locked, w, h,
                          Circle2{ixn_point.P, shapes.snap.distance},
                          get_color(shapes, ixn_point));
      }
    }

    // draw circle around  def_points
    for (const auto &def_point : shapes.def_points) {
      rasterize::circle(pixels_locked, w, h,
                        Circle2{def_point.P, shapes.snap.distance / 3.0},
                        get_color(shapes, def_point));
    }
    // draw circle around  ixn_points
    for (const auto &ixn_point : shapes.ixn_points) {
      rasterize::circle(pixels_locked, w, h,
                        Circle2{ixn_point.P, shapes.snap.distance / 3.0},
                        get_color(shapes, ixn_point));
    }

    // [draw the temporary shape from base to cursor live if in construction]
    if (shapes.construct.shape == ConstructShape::LINE) {
      rasterize::line(pixels_locked, w, h, shapes.construct.line.geom,
                      get_color(shapes, shapes.construct.line));
    }
    if (shapes.construct.shape == ConstructShape::CIRCLE) {
      rasterize::circle(pixels_locked, w, h, shapes.construct.circle.geom,
                        get_color(shapes, shapes.construct.circle));
    }
    if (shapes.construct.shape == ConstructShape::ARC) {
      if (shapes.construct.point_set == PointSet::SECOND) {
        rasterize::arc(pixels_locked, w, h, shapes.construct.arc.geom,
                       get_color(shapes, shapes.construct.arc));
      } else {
        rasterize::line(
            pixels_locked, w, h,
            Line2{shapes.construct.arc.geom.C, shapes.construct.arc.geom.S},
            get_color(shapes, shapes.construct.arc));
      }
    }

    // [draw the edit shape from base to cursor live if in construction]
    if (shapes.edit.in_edit) {
      if (shapes.edit.shape == EditShape::LINE) {
				rasterize::line(pixels_locked, w, h, shapes.edit.line.geom,
                  get_color(shapes, shapes.construct.line));
      }
    }
    SDL_UnlockTexture(app.video.window_texture);
  }
  SDL_RenderTexture(app.video.renderer, app.video.window_texture, NULL, NULL);
  SDL_RenderPresent(app.video.renderer);
  // auto t1 = std::chrono::high_resolution_clock::now();
	// auto t2 = std::chrono::high_resolution_clock::now();
  // std::chrono::duration<double, std::milli> dt_ms = t2 - t1;
  // std::cout << "dt_out: " << dt_ms << std::endl;
}

void check_for_changes(App &app, Shapes &shapes) {
	if (shapes.quantity_change) {
		shapes.recalculate = true;
	} else {
		shapes.recalculate = false;
	}
	shapes.quantity_change = false;
}
