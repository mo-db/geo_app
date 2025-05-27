#include "core.h"
#include "graphics.h"

using namespace std;
namespace graphics {
// Vec2 implementation, Point implementation
Vec2 Vec2::normalize() {
  return {x / sqrt(pow(x, 2.0) + pow(y, 2.0)),
          y / sqrt(pow(x, 2.0) + pow(y, 2.0))};
}

double get_point_point_distance(const Vec2 &p1, const Vec2 &p2) {
  return sqrt(pow(abs(p2.x - p1.x), 2.0) +
                  pow(abs(p2.y - p1.y), 2.0));
}

double points_equal_with_pixel_epsilon(const Vec2 &p1, const Vec2 &p2) {
	return abs(p1.x - p2.x) < pixel_epsilon && abs(p1.y - p2.y) < pixel_epsilon;
}

// Line2 implementation
Vec2 Line2::get_a() const { return Vec2 {p2.y - p1.y, -(p2.x - p1.x)}; }

Vec2 Line2::get_v() const { return Vec2 {p2.x - p1.x, p2.y - p1.y}; }

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
		max(get_point_point_distance(p1, ray_point),
				get_point_point_distance(p2, ray_point));
  return distance_to_far_endpoint <= get_point_point_distance(p1, p2);
}

double Line2::get_distance_point_to_ray(const Vec2 &plane_point) const {
	Vec2 a = get_a();
	return abs((a.x * plane_point.x + a.y * plane_point.y +
									(-a.x * p1.x - a.y * p1.y)) /
								 sqrt(pow(a.x, 2.0) + pow(a.y, 2.0)));
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
  return sqrt(pow((center.x - circum_point.x), 2.0) +
              pow((center.y - circum_point.y), 2.0));
}

double Circle2::get_angle_of_point(const Vec2 &point) const {
  double angle = -std::atan2(point.y - center.y, point.x - center.x);
	if (angle < 0) { angle += 2 * numbers::pi; }
	return angle;
}

void Circle2::set_circum_point(const double &radius) {
	circum_point = {center.x + radius, center.y};
}
void Circle2::set_exact_circum_point(const double &radius, const Vec2 &point) {
	Vec2 v = Line2{center, point}.get_v().normalize();
	circum_point = {v.x * radius, v.y * radius};
}

Vec2 Circle2::project_point(const Vec2 &point) const {
	Vec2 v = Line2{center, point}.get_v().normalize();
	return {center.x + v.x * radius(), center.y + v.y * radius()};
}

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
} // namespace graphics
