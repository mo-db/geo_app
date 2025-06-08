#include "draw.hpp"
namespace draw {
// world to screen conversion
void set_pixel(App &app, uint32_t *pixel_buf, int x, int y, uint32_t color) {
	if (x >= 0 && y >= 0 && x < app.video.w_pixels && y < app.video.h_pixels) {
		pixel_buf[x + y * app.video.w_pixels] = color;
	}
}

void plot_line(App& app, uint32_t *pixel_buf, const Line2 &line, uint32_t color) {
	int x0 = std::round(line.A.x);
	int y0 = std::round(line.A.y);
	int x1 = std::round(line.B.x);
	int y1 = std::round(line.B.y);

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  for (;;) { /* loop */
    set_pixel(app, pixel_buf, x0, y0, color);
    e2 = 2 * err;
    if (e2 >= dy) { /* e_xy+e_x > 0 */
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) { /* e_xy+e_y < 0 */
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}

void plot_arc(App &app, uint32_t *pixel_buf, const Arc2 &arc, uint32_t color) {
	int xm = std::round(arc.C.x);
	int ym = std::round(arc.C.y);
	int r = std::round(arc.radius());
  int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
  do {
		if (arc2::angle_on_arc(arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm - x), static_cast<double>(ym + y)}))) {
			set_pixel(app, pixel_buf, xm - x, ym + y, color); //   I. Quadrant +x +y
		}
		if (arc2::angle_on_arc(arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm - y), static_cast<double>(ym - x)}))) {
			set_pixel(app, pixel_buf, xm - y, ym - x, color); //  II. Quadrant -x +y
		}
		if (arc2::angle_on_arc(arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm + x), static_cast<double>(ym - y)}))) {
			set_pixel(app, pixel_buf, xm + x, ym - y, color); // III. Quadrant -x -y
		}
		if (arc2::angle_on_arc(arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm + y), static_cast<double>(ym + x)}))) {
			set_pixel(app, pixel_buf, xm + y, ym + x, color); //  IV. Quadrant +x -y
		}
    r = err;
    if (r <= y)
      err += ++y * 2 + 1; /* e_xy+e_y < 0 */
    if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
      err += ++x * 2 + 1; /* -> x-step now */
  } while (x < 0);
}

void plot_circle(App &app, uint32_t *pixel_buf, const Circle2 &circle, uint32_t color) {
	int xm = std::round(circle.C.x);
	int ym = std::round(circle.C.y);
	int r = std::round(circle.radius());
  int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
  do {
		set_pixel(app, pixel_buf, xm - x, ym + y, color); //   I. Quadrant +x +y
		set_pixel(app, pixel_buf, xm - y, ym - x, color); //  II. Quadrant -x +y
		set_pixel(app, pixel_buf, xm + x, ym - y, color); // III. Quadrant -x -y
		set_pixel(app, pixel_buf, xm + y, ym + x, color); //  IV. Quadrant +x -y
    r = err;
    if (r <= y)
      err += ++y * 2 + 1; /* e_xy+e_y < 0 */
    if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
      err += ++x * 2 + 1; /* -> x-step now */
  } while (x < 0);
}
uint32_t get_color(const Shapes& shapes, const Shape &shape) {
	if (shapes.ref.shape != RefShape::NONE && shape.id == shapes.ref.id) {
		return special_color;
	}
	// return color hirachical
	if (shape.tflags.selected) {
		return select_color; 
	} else if (shape.tflags.hl_primary) {
		return hl_primary_color;
	} else if (shape.tflags.hl_secondary) {
		return hl_secondary_color;
	} else if (shape.tflags.hl_tertiary) {
		return hl_tertiary_color;
	} else if (shape.pflags.concealed) {
		return conceal_color;
	} else {
		return fg_color;
	}
}
void plot_shapes(App &app, Shapes &shapes) {
	auto t1 = std::chrono::high_resolution_clock::now();
	void *pixels;
	int pitch;
  if (SDL_LockTexture(app.video.window_texture, NULL, &pixels, &pitch)) {
		uint32_t *pixels_locked = (uint32_t *)pixels;
		std::fill_n((uint32_t*)pixels, app.video.w_pixels * app.video.h_pixels, bg_color);

		// [draw all finished shapes]
		for (const auto &line: shapes.lines) {
			plot_line(app, pixels_locked, line.geom, get_color(shapes, line));
		}
		for (const auto &circle: shapes.circles) {
			plot_circle(app, pixels_locked, circle.geom, get_color(shapes, circle));
		}
		for (const auto &arc: shapes.arcs) {
			plot_arc(app, pixels_locked, arc.geom, get_color(shapes, arc));
		}

		// draw circle around snap point
		if (shapes.snap.shape != SnapShape::NONE) {
			plot_circle(app, pixels_locked, Circle2{shapes.snap.point, shapes.snap.distance}, fg_color);
		}

		// draw circle around  ixn_points
		for (const auto &ixn_point : shapes.ixn_points) {
			plot_circle(app, pixels_locked, Circle2{ixn_point.P, shapes.snap.distance},
									get_color(shapes, ixn_point));
		}

		// draw circle around  def_points
		for (const auto &def_point : shapes.def_points) {
			plot_circle(app, pixels_locked, Circle2{def_point.P, shapes.snap.distance},
									get_color(shapes, def_point));
		}

		// [draw the temporary shape from base to cursor live if in construction]
		if (shapes.construct.shape == ConstructShape::LINE) {
			plot_line(app, pixels_locked, shapes.construct.line.geom,
								get_color(shapes, shapes.construct.line));
		}
		if (shapes.construct.shape == ConstructShape::CIRCLE) {
			plot_circle(app, pixels_locked, shapes.construct.circle.geom,
					get_color(shapes, shapes.construct.circle));
		}
		if (shapes.construct.shape == ConstructShape::ARC) {
			if (shapes.construct.point_set == PointSet::SECOND) {
				plot_arc(app, pixels_locked, shapes.construct.arc.geom, 
						get_color(shapes, shapes.construct.arc));
			} else {
				plot_line(app, pixels_locked, 
									Line2{shapes.construct.arc.geom.C,
									shapes.construct.arc.geom.S}, 
									get_color(shapes, shapes.construct.arc));
			}
		}

		// [draw the edit shape from base to cursor live if in construction]
		if (shapes.edit.shape == EditShape::LINE) {
			plot_line(app, pixels_locked, shapes.edit.line.geom,
								get_color(shapes, shapes.construct.line));
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
