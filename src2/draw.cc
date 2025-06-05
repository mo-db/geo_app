#include "draw.hpp"
namespace draw {
// simple version, TODO: inplement Bresenham for better performance
void draw_line(App &app, uint32_t *pixel_buf, const Line2 &line, uint32_t color) {
	int x1 = SDL_lround(line.A.x);
	int y1 = SDL_lround(line.A.y);
	int x2 = SDL_lround(line.B.x);
	int y2 = SDL_lround(line.B.y);
	
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
			if (x >= 0 && x < app.video.w_pixels && y >= 0 && y < app.video.h_pixels) {
				pixel_buf[x + SDL_lround(y) * app.video.w_pixels] = color;
			}
		}
	} else {
		double x;
		int y;
		double m = dx/dy;
		int step = ((y2 > y1) ? +1 : -1);
		for (y = y1; y != y2; y += step) {
			x = m * (double)(y - y1) + (double)x1;
			if (x >= 0 && x < app.video.w_pixels && y >= 0 && y < app.video.h_pixels) {
				pixel_buf[SDL_lround(x) + y * app.video.w_pixels] = color;
			}
		}
	}
}

// TODO: improve simple version for dy sides, TODO: Implement Bresenham
void draw_circle(App &app, uint32_t *pixel_buf, const Circle2 &circle, uint32_t color) {
	double radius = circle.radius();
	for (int x = SDL_lround(circle.C.x - radius); 
			 x < SDL_lround(circle.C.x + radius); x++) {
		double val = SDL_pow((radius), 2.0) - SDL_pow((double)(x - circle.C.x), 2.0);
		if (val < 0) {
			val = 0.0;
		}
		double y_offset = SDL_sqrt(val);
		int y_top = circle.C.y - SDL_lround(y_offset);
		int y_bottom = circle.C.y + SDL_lround(y_offset);

		// Draw the top and bottom points of the circle's circumference:
		if (x >= 0 && x < app.video.w_pixels) {
				if (y_top >= 0 && y_top < app.video.h_pixels)
						pixel_buf[x + y_top * app.video.w_pixels] = color;
				if (y_bottom >= 0 && y_bottom < app.video.h_pixels)
						pixel_buf[x + y_bottom * app.video.w_pixels] = color;
		}
	}
}

void draw_arc(App &app, uint32_t *pixel_buf, const Arc2 &arc, uint32_t color) {
}

uint32_t get_color(const Shapes& shapes, const Shape &shape) {
	if (shapes.ref.shape != RefShape::NONE && shape.id == shapes.ref.id) {
		return ref_color;
	}
	if (shape.selected) {
		return sel_color; 
	} else if (shape.concealed) {
		return conceal_color;
	} else {
		return fg_color;
	}
}
void draw_shapes(App &app, Shapes &shapes) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.video.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.video.w_pixels * app.video.h_pixels, bg_color);

		// [draw all finished shapes]
		for (const auto &line: shapes.lines) {
			draw_line(app, pixels_locked, line.geom, get_color(shapes, line));
		}
		for (const auto &circle: shapes.circles) {
			draw_circle(app, pixels_locked, circle.geom, get_color(shapes, circle));
		}
		for (const auto &arc: shapes.arcs) {
			draw_arc(app, pixels_locked, arc.geom, get_color(shapes, arc));
		}

		// draw circle around snap point
		if (shapes.snap.shape != SnapShape::NONE) {
			draw_circle(app, pixels_locked, Circle2{shapes.snap.point, shapes.snap.distance}, fg_color);
		}

		// draw circle around highlighted ixn_points
		for (const auto &ixn_point : shapes.ixn_points) {
			if (ixn_point.highlighted) {
				draw_circle(app, pixels_locked, Circle2{ixn_point.P, shapes.snap.distance},
										get_color(shapes, ixn_point));
			}
		}

		// draw circle around highlighted def_points
		for (const auto &def_point : shapes.def_points) {
			if (def_point.highlighted) {
				draw_circle(app, pixels_locked, Circle2{def_point.P, shapes.snap.distance},
										get_color(shapes, def_point));
			}
		}

		// draw circle around  ixn_points
		for (const auto &ixn_point : shapes.ixn_points) {
			draw_circle(app, pixels_locked, Circle2{ixn_point.P, shapes.snap.distance},
									get_color(shapes, ixn_point));
		}

		// draw circle around  def_points
		for (const auto &def_point : shapes.def_points) {
			draw_circle(app, pixels_locked, Circle2{def_point.P, shapes.snap.distance},
									get_color(shapes, def_point));
		}

		// [draw the temporary shape from base to cursor live if in construction]
		if (shapes.construct.shape == ConstructShape::LINE) {
			draw_line(app, pixels_locked, shapes.construct.line.geom,
								get_color(shapes, shapes.construct.line));
		}
		if (shapes.construct.shape == ConstructShape::CIRCLE) {
			draw_circle(app, pixels_locked, shapes.construct.circle.geom,
					get_color(shapes, shapes.construct.circle));
		}
		if (shapes.construct.shape == ConstructShape::ARC) {
			if (shapes.construct.point_set == PointSet::FIRST) {
				draw_arc(app, pixels_locked, shapes.construct.arc.geom, 
						get_color(shapes, shapes.construct.arc));
			} else {
				draw_line(app, pixels_locked, 
									Line2{shapes.construct.arc.geom.C,
									shapes.construct.arc.geom.S}, 
									get_color(shapes, shapes.construct.arc));
			}
		}

		SDL_UnlockTexture(app.video.window_texture);
	}
	SDL_RenderTexture(app.video.renderer, app.video.window_texture, NULL, NULL);
	auto t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> 
		dt_ms = t2 - t1;
	// std::cout << "dt_out: " << dt_ms << std::endl;
	SDL_RenderPresent(app.video.renderer);
}
} // namespace draw
