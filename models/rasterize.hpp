// rasterize.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"

namespace rasterize {
void set_pixel(uint32_t *pixel_buf, int w, int h, int x, int y,
							 uint32_t color);
void line(uint32_t *pixel_buf, int w, int h, const Line2 &line,
               uint32_t color);
void circle(uint32_t *pixel_buf, int w, int h, const Circle2 &circle,
                 uint32_t color);
void arc(uint32_t *pixel_buf, int w, int h, const Arc2 &arc,
              uint32_t color);
} // namespace rasterize
