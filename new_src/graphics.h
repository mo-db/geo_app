#pragma once
#include "core.h"

namespace graphics {
	constexpr uint32_t fg_color = 0x00000000;					//black
	constexpr uint32_t bg_color = 0x00FFFFFF;					//white
	constexpr uint32_t hl_color = 0x000000FF;					//blue
	constexpr uint32_t sel_color = 0x00FF0000;				//red
	constexpr uint32_t conceal_color = 0x006c6c6c;		//grey
	constexpr uint32_t gen_color = 0x000000FF;				//blue
	constexpr uint32_t edit_color = 0x0000FF00;				//green
	constexpr double pixel_epsilon = 0.5;

	enum struct ShapeState {
		NORMAL,
		HIGHLIGHTED,
		SELECTED,
		CONCEALED,
		GENSELECTED,
	};

	// Vec2 implementation, Point implementation
	struct Vec2 {
		double x {};
		double y {};
		Vec2 normalize();
	};

	struct IdPoint {
		Vec2 p;
		std::vector<uint32_t> ids;
		ShapeState state = ShapeState::NORMAL;
		IdPoint() = default;
		IdPoint(Vec2 &p, uint32_t id) : p{p}, ids{id} {}
	};

	double get_point_point_distance(const Vec2 &p1, const Vec2 &p2);
	double points_equal_with_pixel_epsilon(const Vec2 &p1, const Vec2 &p2);

	// Line2 implementation
	struct Line2 {
		Vec2 p1 {};
		Vec2 p2 {};
		uint32_t id {};
		ShapeState state { ShapeState::NORMAL };
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
		uint32_t id {};
		ShapeState state = ShapeState::NORMAL;
		Circle2() {}
		Circle2(Vec2 center) : center {center} {}
		Circle2(Vec2 center, Vec2 circum_point)
			: center {center}, circum_point {circum_point} {}
		double radius() const;
		void set_circum_point(const double &radius);
	};

	std::vector<Vec2> Line2_Line2_intersect(const Line2 &l1, const Line2 &l2);
	std::vector<Vec2> Line2_Circle2_intersect(const Line2 &l, const Circle2 &c);
	std::vector<Vec2> Circle2_Circle2_intersect(const Circle2 &c1,
																							const Circle2 &c2);
}
