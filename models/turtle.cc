#include "turtle.hpp"

namespace turtle {
void move(Turtle &turtle) {
	turtle.x += turtle.d * cos(turtle.angle);
	turtle.y += turtle.d * -sin(turtle.angle);
}
void move_draw(Turtle &turtle, uint32_t *pixbuf, int width, int height,
               int color) {
  Vec2 p1{turtle.x, turtle.y};
  move(turtle);
  Vec2 p2{turtle.x, turtle.y};
  rasterize::line(pixbuf, width, height, Line2{p1, p2}, color);
}
void turn_left(Turtle &turtle) { turtle.angle -= turtle.gamma; }
void turn_right(Turtle &turtle) { turtle.angle += turtle.gamma; }
} // namespace turtle
