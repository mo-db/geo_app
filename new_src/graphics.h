#pragma once
#include "core.h"

namespace graphics {
constexpr uint32_t fg_color = 0x00000000;					//black
constexpr uint32_t bg_color = 0x00FFFFFF;					//white
constexpr uint32_t hl_color = 0x00FF0000;					//red
constexpr uint32_t sel_color = 0x00FF0000;				//red
constexpr uint32_t conceal_color = 0x00CCCCCC;		//grey
constexpr uint32_t gen_color = 0x000000FF;				//blue
constexpr uint32_t edit_color = 0x0000FF00;				//green
constexpr double pixel_epsilon = 0.5;

struct ShapeFlags {
	bool selected = false;
	bool concealed = false;
	bool highlighted = false;
	bool padding = false; // explicit padding to make address san not complain
};

// Vec2 implementation, Point implementation
struct Vec2 {
	double x {};
	double y {};
	Vec2 normalize();
};

double get_point_point_distance(const Vec2 &p1, const Vec2 &p2);
double points_equal_with_pixel_epsilon(const Vec2 &p1, const Vec2 &p2);

// IdPoint implementation
struct IdPoint {
	Vec2 p;
	std::vector<int> ids;
	ShapeFlags flags;
	IdPoint() = default;
	IdPoint(Vec2 &p, int id) : p{p}, ids{id} {}
};

// Line2 implementation
struct Line2 {
	Vec2 p1 {};
	Vec2 p2 {};
	ShapeFlags flags;
	int id {};
	Line2() = default;
	Line2(Vec2 p1) : p1 {p1} {}
	Line2(Vec2 p1, Vec2 p2) : p1 {p1}, p2 {p2} {}

	Vec2 get_a() const;
	Vec2 get_v() const;
	Vec2 project_point_to_ray(const Vec2 &plane_point) const;
	bool check_point_within_seg_bounds(const Vec2 &point) const;
	double get_distance_point_to_ray(const Vec2 &plane_point) const;
	double get_distance_point_to_seg(const Vec2 &plane_point) const;
};

struct Circle2 {
	Vec2 center {};
	Vec2 circum_point {};
	int id {};
	ShapeFlags flags;
	Circle2() = default;
	Circle2(Vec2 center) : center {center} {}
	Circle2(Vec2 center, Vec2 circum_point)
		: center {center}, circum_point {circum_point} {}
	Circle2(Vec2 center, double radius) : center{center} {
		circum_point = {center.x + radius, center.y};
	}
	double radius() const;
	double get_angle_of_point(const Vec2 &p) const;
	Vec2 project_point(const Vec2 &p) const;
	void set_circum_point(const double &radius);
	void set_exact_circum_point(const double &radius, const Vec2 &point);
};

struct Arc2 : public Circle2 {
	// Circle2 circle;
	Vec2 end_point {};
	double start_angle {};  // computed from circum_point
	double end_angle {};    // computed from end_pt
	bool clockwise = true;
	
	// constructors are not inherited by default
	Arc2() = default;
	Arc2(Vec2 center, Vec2 circum_point, Vec2 end_pt)
			: Circle2(center, circum_point), end_point(end_pt) {
		start_angle = atan2(circum_point.y - center.y, 
												circum_point.x - center.x);
		end_angle = atan2(end_pt.y - center.y, end_pt.x - center.x);
	}
	bool angle_between_arc_points(const double &angle) const;
};

std::vector<Vec2> Line2_Line2_intersect(const Line2 &l1, const Line2 &l2);
std::vector<Vec2> Line2_Circle2_intersect(const Line2 &l, const Circle2 &c);
std::vector<Vec2> Circle2_Circle2_intersect(const Circle2 &c1, const Circle2 &c2);
std::vector<Vec2> Arc2_Line2_intersect(const Arc2 &a, const Line2 &l);
std::vector<Vec2> Arc2_Circle2_intersect(const Arc2 &a, const Circle2 &c);
std::vector<Vec2> Arc2_Arc2_intersect(const Arc2 &a1, const Arc2 &a2);
} // namespace graphics
