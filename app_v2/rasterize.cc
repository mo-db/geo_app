#include "rasterize.hpp"

namespace rasterize {
// world to screen conversion
void set_pixel(uint32_t *pixel_buf, int w, int h, int x, int y, uint32_t color) {
	if (x >= 0 && y >= 0 && x < w && y < h) {
		pixel_buf[x + y * w] = color;
	}
}


void line(uint32_t *pixel_buf, int w, int h, const Line2 &line,
               uint32_t color) {
  int x0 = std::round(line.A.x);
  int y0 = std::round(line.A.y);
  int x1 = std::round(line.B.x);
  int y1 = std::round(line.B.y);

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  for (;;) { /* loop */
    set_pixel(pixel_buf, w, h, x0, y0, color);
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


void circle(uint32_t *pixel_buf, int w, int h, const Circle2 &circle,
            uint32_t color) {
  int xm = std::round(circle.C.x);
  int ym = std::round(circle.C.y);
  int r = std::round(circle.radius());
  int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
  do {
    set_pixel(pixel_buf, w, h, xm - x, ym + y, color); //   I. Quadrant +x +y
    set_pixel(pixel_buf, w, h, xm - y, ym - x, color); //  II. Quadrant -x +y
    set_pixel(pixel_buf, w, h, xm + x, ym - y, color); // III. Quadrant -x -y
    set_pixel(pixel_buf, w, h, xm + y, ym + x, color); //  IV. Quadrant +x -y
    r = err;
    if (r <= y)
      err += ++y * 2 + 1; /* e_xy+e_y < 0 */
    if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
      err += ++x * 2 + 1; /* -> x-step now */
  } while (x < 0);
}


void arc(uint32_t *pixel_buf, int w, int h, const Arc2 &arc,
              uint32_t color) {
  int xm = std::round(arc.C.x);
  int ym = std::round(arc.C.y);
  int r = std::round(arc.radius());
  int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
  do {
    if (arc2::angle_on_arc(
            arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm - x),
                                             static_cast<double>(ym + y)}))) {
      set_pixel(pixel_buf, w, h, xm - x, ym + y, color); //   I. Quadrant +x +y
    }
    if (arc2::angle_on_arc(
            arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm - y),
                                             static_cast<double>(ym - x)}))) {
      set_pixel(pixel_buf, w, h, xm - y, ym - x, color); //  II. Quadrant -x +y
    }
    if (arc2::angle_on_arc(
            arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm + x),
                                             static_cast<double>(ym - y)}))) {
      set_pixel(pixel_buf, w, h, xm + x, ym - y, color); // III. Quadrant -x -y
    }
    if (arc2::angle_on_arc(
            arc, vec2::get_angle(arc.C, Vec2{static_cast<double>(xm + y),
                                             static_cast<double>(ym + x)}))) {
      set_pixel(pixel_buf, w, h, xm + y, ym + x, color); //  IV. Quadrant +x -y
    }
    r = err;
    if (r <= y)
      err += ++y * 2 + 1; /* e_xy+e_y < 0 */
    if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
      err += ++x * 2 + 1; /* -> x-step now */
  } while (x < 0);
}
} // namespace rasterize
