#pragma once
#include "core.hpp"
#include "graphics.hpp"
#include "rasterize.hpp"

struct Turtle {
  double x = 0.0;
  double y = 0.0;
  double angle = 0.0;
  double d = 0.0;
  double gamma = 0.0;
  Turtle() = default;
  Turtle(const double x, const double y, const double angle, const double d,
         const double gamma)
      : x{x}, y{y}, angle{angle}, d{d}, gamma{gamma} {}
};

namespace turtle {
void move_draw(Turtle &turtle, uint32_t *pixbuf, int width, int height,
               int color);
void move(Turtle &turtle);
void turn_left(Turtle &turtle);
void turn_right(Turtle &turtle);
} // namespace turtle
