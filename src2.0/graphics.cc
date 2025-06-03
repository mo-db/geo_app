#include "graphics.hpp"

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
double dot(const Vec2 &a, const Vec2 &b) {
	return a.x * b.x + a.y * b.y;
}
double distance(const Vec2 &a, const Vec2 &b) {
  return std::sqrt(std::pow(a.x - b.x, 2.0) + std::pow(a.y - b.y, 2.0));
}
bool test_equal(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::int_epsilon &&
         std::abs(a.y - b.y) < gk::int_epsilon;
}
bool test_equal_precise(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::epsilon &&
				 std::abs(a.y - b.y) < gk::epsilon;
}
} // namespace vec2

namespace line2 {
Vec2 project_point(const Line2 &line, const Vec2 &P) {
	Vec2 v = line.get_v();
	Vec2 AP = P - line.A;
	double t = vec2::dot(AP, v) / vec2::dot(v, v);
	return line.A + v * t;
}

bool point_in_segment_bounds(const Line2 &line, const Vec2 &P) {
  double distance_to_far_endpoint = std::max(vec2::distance(line.A, P),
																						 vec2::distance(line.B, P));
  return distance_to_far_endpoint <= vec2::distance(line.A, line.B);
}

double get_distance_point_to_ray(const Line2 &line, const Vec2 &P) {
	Vec2 a = line.get_a();
	return std::abs((a.x * P.x + a.y * P.y +
									(-a.x * line.A.x - a.y * line.B.y)) /
									std::sqrt(std::pow(a.x, 2.0) + std::pow(a.y, 2.0)));
}
double get_distance_point_to_seg(const Line2 &line, const Vec2 &P) {
  Vec2 projected_point = project_point(line, P);
  if (point_in_segment_bounds(line, projected_point)) {
    return get_distance_point_to_ray(line, P);
  } else {
    return std::min(vec2::distance(P, line.A), vec2::distance(P, line.B));
  }
}
} // namespace line2

namespace circle2 {
double get_angle_of_point(Circle2 &circle, const Vec2 &P) {
	Vec2 v = P - circle.C;
  double angle = -std::atan2(v.y, v.x); // because 0/0 is up left
	if (angle < 0) { angle += 2 * std::numbers::pi; }
	return angle;
}

Vec2 project_point(const Circle2 &circle, const Vec2 &P) {
	Vec2 v = (P - circle.C).norm();
	double r = circle.radius();
	return circle.C + v * r;
}

void set_P(Circle2 &circle, const double &radius) {
	circle.P = {circle.C.x + radius, circle.C.y};
}

void set_exact_P(Circle2 &circle, const double &radius, const Vec2 &P) {
	Vec2 v = (P - circle.C).norm();
	circle.P = circle.C + v * radius;
}
} // namespace circle2

namespace arc2 {
bool angle_on_arc(const Arc2& arc, const double &angle) {
	if (arc.clockwise) {
		if (arc.E_angle < arc.S_angle) {
			return angle < arc.S_angle && angle > arc.E_angle;
		} else {
			return angle < arc.S_angle || angle > arc.E_angle;
		}
	} else {
		if (arc.E_angle > arc.S_angle) {
			return angle < arc.S_angle && angle > arc.E_angle;
		} else {
			return angle < arc.S_angle || angle > arc.E_angle;
		}
	}
}
} // namespace arc2

namespace graphics {
std::vector<Vec2> Line2_Line2_intersect(const Line2 &l1, const Line2 &l2) {
  Vec2 l1_a = l1.get_a();
  Vec2 l2_v = l2.get_v();
	// calculate the intersection point, check if denominator nears 0
	double numerator = (l1_a.x * l1.p1.x + l1_a.y * l1.p1.y) -
											l1_a.x * l2.p1.x -l1_a.y * l2.p1.y;
	double denominator = l1_a.x * l2_v.x + l1_a.y * l2_v.y;
  if (std::abs(denominator) < gk::epsilon) { return {}; }
  double k = numerator / denominator;
  Vec2 ixn_point{l2.p1.x + k * l2_v.x, l2.p1.y + k * l2_v.y};

	// return if point is in line segment bounds
  if (l1.check_point_within_seg_bounds(ixn_point) &&
      l2.check_point_within_seg_bounds(ixn_point)) {
    return vector<Vec2>{ixn_point};
  } else {
    return vector<Vec2>{};
  }
}

std::vector<Vec2> Line2_Circle2_intersect(const Line2 &l, const Circle2 &c) {
	Vec2 v_normal = (l.get_v()).normalize();
	double distance = l.get_distance_point_to_ray(c.center);
	// TODO maybe first check for equal with pixel_epsilon, then < 
	if (distance < c.radius()) {
		Vec2 center_to_line_projection = l.project_point_to_ray(c.center);
		double hight = sqrt(abs(pow(c.radius(), 2.0) - pow(distance, 2.0)));
		Vec2 ixn_point_1 { center_to_line_projection.x + hight * v_normal.x, 
			center_to_line_projection.y + hight * v_normal.y};
		Vec2 ixn_point_2 { center_to_line_projection.x - hight * v_normal.x, 
			center_to_line_projection.y - hight * v_normal.y};

		// return ixn_points if within line segment bounds
		vector<Vec2> ixn_points {};
		if (l.check_point_within_seg_bounds(ixn_point_1)) {
			ixn_points.push_back(ixn_point_1);
		}
		if (l.check_point_within_seg_bounds(ixn_point_2)) {
			ixn_points.push_back(ixn_point_2);
		}
		return ixn_points;
	} else {
		return vector<Vec2>{};
	}
}

std::vector<Vec2> Circle2_Circle2_intersect(const Circle2 &c1,
																						const Circle2 &c2) {
	// check if circles overlap
	double c1_radius = c1.radius();
	double c2_radius = c2.radius();
	double center_distance = get_point_point_distance(c1.center, c2.center);

	if (center_distance < (c1.radius() + c2.radius()) &&
			min(c1.radius(), c2.radius()) >
			(max(c1.radius(), c2.radius()) - center_distance)) {
		// test if one circle is fully inside the other circle
		if (center_distance < max(c1_radius, c2_radius)) {
			if (min(c1.radius(), c2.radius()) <
					(max(c1.radius(), c2.radius()) - center_distance)) {
				return vector<Vec2> {};
			}
		}
		double meet_distance =
			(pow(c1.radius(), 2.0) - pow(c2.radius(), 2.0) +
      pow(center_distance, 2.0)) / (2 * center_distance);
		double h = sqrt(pow(c1.radius(), 2.0) - pow(meet_distance, 2.0));
		Line2 center_center_line {c1.center, c2.center};
		Vec2 v_normal = center_center_line.get_v().normalize();
		Vec2 a_normal = center_center_line.get_a().normalize();
		Vec2 meet_point = {c1.center.x + v_normal.x * meet_distance,
											 c1.center.y + v_normal.y *meet_distance};
		Vec2 ixn_point_1 = {meet_point.x + h * a_normal.x,
												meet_point.y + h *a_normal.y};
		Vec2 ixn_point_2 = {meet_point.x - h * a_normal.x,
												meet_point.y - h *a_normal.y};
		return vector<Vec2> {ixn_point_1, ixn_point_2};
	} else {
		return vector<Vec2> {};
	}
}
std::vector<Vec2> Arc2_Line2_intersect(const Arc2 &a, const Line2 &l) {
	vector<Vec2> ixn_points = Line2_Circle2_intersect(l, a);
	for (auto iter = ixn_points.begin(); iter != ixn_points.end();) {
		// careful with iterator erase and incrementing
		if (!a.angle_between_arc_points(a.get_angle_of_point(*iter))) {
			iter = ixn_points.erase(iter);
		} else {
			iter++;
		}
	}
	return ixn_points;
}
std::vector<Vec2> Arc2_Circle2_intersect(const Arc2 &a, const Circle2 &c) {
	vector<Vec2> ixn_points = Circle2_Circle2_intersect(c, static_cast<Circle2>(a));
	for (auto iter = ixn_points.begin(); iter != ixn_points.end();) {
		if (!a.angle_between_arc_points(a.get_angle_of_point(*iter))) {
			iter = ixn_points.erase(iter);
		} else {
			iter++;
		}
	}
	return ixn_points;
}
std::vector<Vec2> Arc2_Arc2_intersect(const Arc2 &a1,	const Arc2 &a2) {
	vector<Vec2> ixn_points =
		Circle2_Circle2_intersect(static_cast<Circle2>(a1),
															static_cast<Circle2>(a2));
	for (auto iter = ixn_points.begin(); iter != ixn_points.end();) {
		if (!a1.angle_between_arc_points(a1.get_angle_of_point(*iter))) {
			iter = ixn_points.erase(iter);
		} else {
			iter++;
		}
	}
	return ixn_points;
}

}
