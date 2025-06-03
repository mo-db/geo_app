#include "shapes.hpp"

LineShape *Shapes::get_line_by_id(const int id) {
	for (auto &line : lines) {
		if (id == line.id) {
			return &line;
		}
	}
	return nullptr;
}
CircleShape *Shapes::get_circle_by_id(const int id) {
	for (auto &circle : circles) {
		if (id == circle.id) {
			return &circle;
		}
	}
	return nullptr;
}
ArcShape *Shapes::get_arc_by_id(const int id) {
	for (auto &arc : arcs) {
		if (id == arc.id) {
			return &arc;
		}
	}
	return nullptr;
}

namespace shapes {

void construct_line(App &app, Shapes &shapes, const Vec2 &P) {
	auto &line_shape = shapes.construct.line_shape;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::LINE;
			line_shape.line.A = P;
			line_shape.concealed  = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			if (shapes.ref.shape == RefShape::LINE) {
				double ref_length = shapes.ref.value;
				assert(ref_length >= 0);
				Vec2 v_norm = Line2{line_shape.line.A, P}.get_v().norm();
				line_shape.line.B = line_shape.line.B + v_norm.x * ref_length;
			} else {
				line_shape.line.B = P;
			}
			line_shape.concealed = construct.concealed;
			line_shape.id = shapes.id_counter++;
			shapes.lines.push_back(line_shape);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (construct.point_set == PointSet::FIRST) {
		if (shapes.ref.shape == RefShape::LINE) {
			double ref_length = shapes.ref.value;
			assert(ref_length >= 0);
			Vec2 v_norm = Line2{line_shape.line.A, P}.get_v().norm();
			line_shape.line.B = line_shape.line.B + v_norm.x * ref_length;
		} else {
			line_shape.line.B = P;
		}
		line_shape.line.B = P;
	}
}

void construct_circle(App &app, Shapes &shapes, const Vec2 &P) {
	auto &circle_shape = shapes.construct.circle_shape;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::CIRCLE;
			circle_shape.circle.C = P;
			circle_shape.concealed = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			if (shapes.ref.shape == RefShape::CIRCLE) {
				double ref_radius = shapes.ref.value;
				assert(ref_radius >= 0);
				circle2::set_exact_P(circle_shape.circle, ref_radius, P);
			} else {
				circle_shape.circle.P = P;
			}
			circle_shape.concealed = construct.concealed;
			circle_shape.id = shapes.id_counter++;
			shapes.circles.push_back(circle_shape);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		if (shapes.ref.shape == RefShape::CIRCLE) {
			double ref_radius = shapes.ref.value;
			assert(ref_radius >= 0);
			circle2::set_exact_P(circle_shape.circle, ref_radius, P);
		} else {
			circle_shape.circle.P = P;
		}
	}
}

void construct_arc(AppState &app, Vec2 const &pt) {
	if (app.mouse_click) {
		if (shapes.construct.point_set == PointSet::NONE) {
			shapes.construct.point_set = PointSet::FIRST;
			in_construction = InConstruction::ARC;
			temp_arc.center = pt;
			temp_arc.flags.concealed = construct_concealed;
		} else if (shapes.construct.point_set == PointSet::FIRST) {
			shapes.construct.point_set = PointSet::SECOND;
			if (app.shift_set) {
				double last_radius = radius_last_selected_circle();
				if (last_radius >= 0.) {
					temp_arc.set_exact_circum_point(last_radius, pt);
				} else {
					temp_arc.circum_point = pt;
				}
			} else {
				temp_arc.circum_point = pt;
			}
		} else if (shapes.construct.point_set == PointSet::SECOND) {
			// if snap to shape, calculate ixn_point and use this to snap arc
			if (in_snap_distance && !snap_is_id_point) {
				if (snap_shape == SnapShape::LINE) {
					Line2 &snap_line = lines[snap_index];
					vector<Vec2> ixn_points = graphics::Arc2_Line2_intersect(temp_arc, snap_line);
					if (ixn_points.size() > 1) {
						if (graphics::get_point_point_distance(ixn_points.at(0), app.mouse) < 
								graphics::get_point_point_distance(ixn_points.at(1), app.mouse)) {
							temp_arc.end_point = ixn_points.at(0);
						} else {
							temp_arc.end_point = ixn_points.at(1);
						}
					} else {
						temp_arc.end_point = ixn_points.back();
					}
				} else if (snap_shape == SnapShape::CIRCLE) {
					Circle2 &snap_circle = circles[snap_index];
					vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(temp_arc, snap_circle);
					if (ixn_points.size() > 1) {
						if (graphics::get_point_point_distance(ixn_points.at(0), app.mouse) < 
								graphics::get_point_point_distance(ixn_points.at(1), app.mouse)) {
							temp_arc.end_point = ixn_points.at(0);
						} else {
							temp_arc.end_point = ixn_points.at(1);
						}
					} else {
						temp_arc.end_point = ixn_points.back();
					}
				} else if (snap_shape == SnapShape::ARC) {
					Arc2 &snap_arc = arcs[snap_index];
					vector<Vec2> ixn_points = graphics::Arc2_Arc2_intersect(temp_arc, snap_arc);
					if (ixn_points.size() > 1) {
						if (graphics::get_point_point_distance(ixn_points.at(0), app.mouse) < 
								graphics::get_point_point_distance(ixn_points.at(1), app.mouse)) {
							temp_arc.end_point = ixn_points.at(0);
						} else {
							temp_arc.end_point = ixn_points.at(1);
						}
					} else {
						temp_arc.end_point = ixn_points.back();
					}
				}
			} else {
				temp_arc.end_point = temp_arc.project_point(pt);
			}

			temp_arc.start_angle = temp_arc.get_angle_of_point(temp_arc.circum_point);
			temp_arc.end_angle = temp_arc.get_angle_of_point(temp_arc.end_point);
			temp_arc.flags.concealed = construct_concealed;
			temp_arc.id = id_counter++;
			arcs.push_back(temp_arc);
			quantity_change = true;
			clear_construction();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		if (app.shift_set) {
			double last_radius = radius_last_selected_circle();
			if (last_radius >= 0.) {
				temp_arc.set_exact_circum_point(last_radius, pt);
			} else {
				temp_arc.circum_point = pt;
			}
		} else {
			temp_arc.circum_point = pt;
		}
	} else if (shapes.construct.point_set == PointSet::SECOND) {
		temp_arc.end_point = temp_arc.project_point(pt);
		Vec2 v1 = {temp_arc.circum_point.x - temp_arc.center.x,
							 temp_arc.circum_point.y - temp_arc.center.y};
		Vec2 v2 = {temp_arc.end_point.x - temp_arc.center.x,
							 temp_arc.end_point.y - temp_arc.center.y};
		double cross = v1.x * v2.y - v1.y * v2.x;
		cout << "cross: " << cross << endl;
	}
}

void construct(AppState &app, Vec2 const &pt) {
  switch (app.mode) {
    case AppMode::NORMAL:    return;
    case AppMode::LINE:      construct_line(app, pt);   break;
    case AppMode::CIRCLE:    construct_circle(app, pt); break;
    case AppMode::ARC:       construct_arc(app, pt);    break;
		default:								 return;
  }
}

bool update_snap(AppState &app) {
	snap_index = -1;
	snap_point = {};
	in_snap_distance = false;
	snap_is_id_point = false;
	snap_shape = SnapShape::NONE;

	if (snap_to_id_pts) {
		for (size_t index = 0; index < ixn_points.size(); index++) {
			if (get_point_point_distance(ixn_points[index].p, app.mouse) < snap_distance) {
				snap_point = ixn_points[index].p;
				snap_shape = SnapShape::IXN_POINT;
				snap_is_id_point = true;
				snap_index = index;
				return true;
			}	
		}

		for (size_t index = 0; index < def_points.size(); index++) {
			if (get_point_point_distance(def_points[index].p, app.mouse) < snap_distance) {
				snap_point = def_points[index].p;
				snap_shape = SnapShape::DEF_POINT;
				snap_is_id_point = true;
				snap_index = index;
				return true;
			}	
		}
	}

	for (size_t index = 0; index < lines.size(); index++) {
		Line2 &line = lines[index];
		if (line.get_distance_point_to_seg(app.mouse) < snap_distance) {
			Vec2 projected_point = line.project_point_to_ray(app.mouse);
			double p1_distance = get_point_point_distance(app.mouse, line.p1);
			double p2_distance = get_point_point_distance(app.mouse, line.p2);
			if (line.check_point_within_seg_bounds(projected_point)) {
				snap_point = projected_point;
				snap_shape = SnapShape::LINE;
				snap_is_id_point = false;
				snap_index = index;
				return true;
			} else if (p1_distance < snap_distance) {
				snap_point = line.p1;
				snap_shape = SnapShape::LINE;
				snap_is_id_point = false;
				snap_index = index;
				return true;
			} else if (p2_distance < snap_distance) {
				snap_point = line.p2;
				snap_shape = SnapShape::LINE;
				snap_is_id_point = false;
				snap_index = index;
				return true;
			}
		}
	}

	for (size_t index = 0; index < circles.size(); index++) {
		Circle2 &circle = circles[index];
		double distance = get_point_point_distance(circle.center, app.mouse);
		if (distance < circle.radius() + snap_distance &&
				distance > circle.radius() - snap_distance) {
			snap_point = circle.project_point(app.mouse);
			snap_shape = SnapShape::CIRCLE;
			snap_is_id_point = false;
			snap_index = index;
			return true;
		}
	}

	for (size_t index = 0; index < arcs.size(); index++) {
		Arc2 &arc = arcs[index];
		double distance = get_point_point_distance(arc.center, app.mouse);
		if (distance < arc.radius() + snap_distance &&
				distance > arc.radius() - snap_distance) {
			if (arc.angle_between_arc_points(arc.get_angle_of_point(app.mouse))) {
			snap_point = arc.project_point(app.mouse);
			snap_shape = SnapShape::ARC;
			snap_is_id_point = false;
			snap_index = index;
			return true;
			}
		}
	}
	return false;
}
} // namespace shapes
