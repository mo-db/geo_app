#include "include.h"
#include "graphics.h"

using namespace std;
using namespace graphics;

// Vec2 implementation, Point implementation
Vec2 Vec2::normalize() {
  return {x / SDL_sqrt(SDL_pow(x, 2.0) + SDL_pow(y, 2.0)),
          y / SDL_sqrt(SDL_pow(x, 2.0) + SDL_pow(y, 2.0))};
}

double get_point_point_distance(const Vec2 &p1, const Vec2 &p2) {
  return SDL_sqrt(SDL_pow(SDL_fabs(p2.x - p1.x), 2.0) +
                  SDL_pow(SDL_fabs(p2.y - p1.y), 2.0));
}

// Line2 implementation
Vec2 Line2::get_a() const { return Vec2 {p2.y - p1.y, -(p2.x - p1.x)}; }

Vec2 Line2::get_v() const { return Vec2 {p2.x - p1.x, p2.y - p2.x}; }

Vec2 Line2::project_point_to_ray(const Vec2 &plane_point) const {
	Vec2 a = get_a();
	Vec2 line_point {};
	double k = ((p1.x * a.x + p1.y * a.y) -
							(plane_point.x * a.x + plane_point.y * a.y)) /
						 (a.x * a.x + a.y * a.y);
	line_point.x = k * a.x + plane_point.x;
	line_point.y = k * a.y + plane_point.y;
	return line_point;
}

bool Line2::check_point_within_seg_bounds(const Vec2 &ray_point) const {
  double distance_to_far_endpoint =
		std::max(get_point_point_distance(p1, ray_point),
						 get_point_point_distance(p2, ray_point));
  return distance_to_far_endpoint <= get_point_point_distance(p1, p2);
}

double Line2::get_distance_point_to_ray(const Vec2 &plane_point) const {
	Vec2 a = get_a();
	return SDL_fabs((a.x * plane_point.x + a.y * plane_point.y +
									(-a.x * p1.x - a.y * p1.y)) /
								 SDL_sqrt(SDL_pow(a.x, 2.0) + SDL_pow(a.y, 2.0)));
}

double Line2::get_distance_point_to_seg(const Vec2 &plane_point) const {
  Vec2 projected_point = project_point_to_ray(plane_point);
  if (check_point_within_seg_bounds(projected_point)) {
    return get_distance_point_to_ray(plane_point);
  } else {
    return min(get_point_point_distance(plane_point, p1),
               get_point_point_distance(plane_point, p2));
  }
}

// Circle2 implementation
double Circle2::radius() const {
	return SDL_sqrt(SDL_pow((center.x - circum_point.x), 2.0) +
													 SDL_pow((center.y - circum_point.y), 2.0));
}


void Circle2::set_circum_point(const double &radius) {
	circum_point = { center.x + radius, center.y };
}
