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

// operator overloads
Vec2 operator+(const Vec2 &a, const Vec2 &b) {
	return {a.x + b.x, a.y + b.y};
}
Vec2 operator+(const Vec2 &a, const double d) {
	return {a.x + d, a.y + d};
}
Vec2 operator-(const Vec2 &a, const Vec2 &b) {
	return {a.x - b.x, a.y - b.y};
}
Vec2 operator*(const Vec2 &v, double d) {
	return {v.x * d, v.y * d};
}
Vec2 operator*(double d, const Vec2 &v) {
	return v * d;
}

namespace vec2 {
double vec2_dot(const Vec2 &a, const Vec2 &b) {
	return a.x * b.x + a.y * b.y;
}
double vec2_distance(const Vec2 &a, const Vec2 &b) {
  return std::sqrt(std::pow(a.x - b.x, 2.0) + std::pow(a.y - b.y, 2.0));
}
bool vec2_test_equal(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::int_epsilon &&
         std::abs(a.y - b.y) < gk::int_epsilon;
}
bool vec2_test_equal_precise(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::epsilon &&
				 std::abs(a.y - b.y) < gk::epsilon;
}
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
Vec2 line2_project_point(const Vec2 &P);
bool line2_test_point_within_seg_bounds(const Vec2 &point);
double get_distance_point_to_ray(const Vec2 &plane_point);
double get_distance_point_to_seg(const Vec2 &plane_point);
} // namespace line2

struct Circle2 {
	Vec2 C{}, P{};
	Circle2() = default;
	Circle2(const Vec2 C, const Vec2 P) : C{C}, P{P} {}
	double radius() const {
		return sqrt(pow((C.x - P.x), 2.0) + pow((C.y - P.y), 2.0));
	}
};

namespace circle2 {
double get_angle_of_point(Circle2 &circle, const Vec2 &P);
Vec2 project_point(Circle2 &circle, const Vec2 &P);
void set_P(Circle2 &circle, const double &radius);
void set_exact_P(Circle2 &circle, const double &radius, const Vec2 &P);
} // namespace circle2

struct Arc2 {
	Vec2 C{}, S{}, E{};
	Arc2() = default;
	Arc2(const Vec2 C, const Vec2 S, const Vec2 E) : C{C}, S{S}, E{E} {}
	double radius() const {
		return sqrt(pow((C.x - S.x), 2.0) + pow((C.y - S.y), 2.0));
	}
	Circle2 to_circle() {
		return Circle2{C, S};
	}
};
namespace arc2 {
} // namespace arc2

namespace graphics {
constexpr uint32_t fg_color = 0x00000000;					//black
constexpr uint32_t bg_color = 0x00FFFFFF;					//white
constexpr uint32_t hl_color = 0x00FF0000;					//red
constexpr uint32_t sel_color = 0x00FF0000;				//red
constexpr uint32_t conceal_color = 0x00CCCCCC;		//grey
constexpr uint32_t gen_color = 0x000000FF;				//blue
constexpr uint32_t edit_color = 0x0000FF00;				//green
																									//
std::vector<Vec2> Line2_Line2_intersect(const Line2 &l1, const Line2 &l2);
std::vector<Vec2> Line2_Circle2_intersect(const Line2 &l, const Circle2 &c);
std::vector<Vec2> Circle2_Circle2_intersect(const Circle2 &c1, const Circle2 &c2);
std::vector<Vec2> Arc2_Line2_intersect(const Arc2 &a, const Line2 &l);
std::vector<Vec2> Arc2_Circle2_intersect(const Arc2 &a, const Circle2 &c);
std::vector<Vec2> Arc2_Arc2_intersect(const Arc2 &a1, const Arc2 &a2);
} // namespace graphics
