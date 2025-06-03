// graphics.hpp
#pragma once
#include "core.hpp"

struct Vec2 {
  double x{0.0}, y{0.0};
  Vec2() = default;
  Vec2(const double x, const double y) : x{x}, y{y} {}
  // magnitude
  double mag() {
		return std::sqrt(x * x + y * y);
	}
  // normalize
  Vec2 norm() {
    double m = mag();
    return {x / m, y / m};
  }
};

Vec2 operator+(const Vec2 &a, const Vec2 &b);
Vec2 operator+(const Vec2 &a, const double d);
Vec2 operator-(const Vec2 &a, const Vec2 &b);
Vec2 operator*(const Vec2 &v, double d);
Vec2 operator*(double d, const Vec2 &v);

namespace vec2 {
double dot(const Vec2 &a, const Vec2 &b);
double distance(const Vec2 &a, const Vec2 &b);
bool equal_int_epsilon(const Vec2 &a, const Vec2 &b);
bool equal_epsilon(const Vec2 &a, const Vec2 &b);
} // namespace vec2

struct Line2 {
	Vec2 A{}, B{};
	Line2() = default;
	Line2(const Vec2 A) : A{A} {}
	Line2(const Vec2 A, const Vec2 B) : A{A}, B{B} {}
	Vec2 get_a() const { return Vec2 {B.y - A.y, -(B.x - A.x)}; }
	Vec2 get_v() const { return Vec2 {B.x - A.x, B.y - A.y}; }
};

namespace line2 {
Vec2 project_point(const Line2 &line, const Vec2 &P);
bool point_in_segment_bounds(const Line2 &line, const Vec2 &P);
double get_distance_point_to_ray(const Line2 &line, const Vec2 &P);
double get_distance_point_to_seg(const Line2 &line, const Vec2 &P);
} // namespace line2

struct Circle2 {
	Vec2 C{}, P{};
	Circle2() = default;
	Circle2(const Vec2 C, const Vec2 P) : C{C}, P{P} {}
	Circle2(const Vec2 C, const double d) : C{C} { P = {C.x + d, C.y}; }
	double radius() const {
		return sqrt(pow((C.x - P.x), 2.0) + pow((C.y - P.y), 2.0));
	}
};

namespace circle2 {
double get_angle_of_point(const Circle2 &circle, const Vec2 &P);
Vec2 project_point(const Circle2 &circle, const Vec2 &P);
void set_P(Circle2 &circle, const double &radius);
void set_exact_P(Circle2 &circle, const double &radius, const Vec2 &P);
} // namespace circle2

struct Arc2 {
	Vec2 C{}, S{}, E{};
	double S_angle{}, E_angle{};
	bool clockwise = true;
	Arc2() = default;
	Arc2(const Vec2 C, const Vec2 S, const Vec2 E) : C{C}, S{S}, E{E} {}
	double radius() const {
		return sqrt(pow((C.x - S.x), 2.0) + pow((C.y - S.y), 2.0));
	}
	Circle2 to_circle() const {
		return Circle2{C, S};
	}
};
namespace arc2 {
bool angle_on_arc(const Arc2& arc, const double &angle);
void set_S(Arc2 &arc, const double &radius, const Vec2 &P);
} // namespace arc2

namespace graphics {


std::vector<Vec2> Line2_Line2_intersect(const Line2 &l1, const Line2 &l2);
std::vector<Vec2> Line2_Circle2_intersect(const Line2 &l, const Circle2 &c);
std::vector<Vec2> Circle2_Circle2_intersect(const Circle2 &c1, const Circle2 &c2);
std::vector<Vec2> Arc2_Line2_intersect(const Arc2 &a, const Line2 &l);
std::vector<Vec2> Arc2_Circle2_intersect(const Arc2 &a, const Circle2 &c);
std::vector<Vec2> Arc2_Arc2_intersect(const Arc2 &a1, const Arc2 &a2);
} // namespace graphics
