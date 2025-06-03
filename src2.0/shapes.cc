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

void construct_line(const App &app, Shapes &shapes, const Vec2 &P) {
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

void set_P(Shapes &shapes, Circle2 &circle, const Vec2 &P) {
		if (shapes.ref.shape == RefShape::CIRCLE) {
			double ref_radius = shapes.ref.value;
			assert(ref_radius >= 0);
			circle2::set_exact_P(circle, ref_radius, P);
		} else {
			circle.P = P;
		}
}

void construct_circle(const App &app, Shapes &shapes, const Vec2 &P) {
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
			set_P(shapes, circle_shape.circle, P);

			circle_shape.concealed = construct.concealed;
			circle_shape.id = shapes.id_counter++;
			shapes.circles.push_back(circle_shape);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		set_P(shapes, circle_shape.circle, P);
	}
}

// arc
void set_snap_E(std::vector<Vec2> ixn_points, const App &app, ArcShape &arc_shape) {
	if (ixn_points.size() > 1) {
		if (vec2::distance(ixn_points.at(0), app.input.mouse) < 
				vec2::distance(ixn_points.at(1), app.input.mouse)) {
			arc_shape.arc.E = ixn_points.at(0);
		} else {
			arc_shape.arc.E = ixn_points.at(1);
		}
	} else {
		arc_shape.arc.E = ixn_points.back();
	} 
}

void set_E(const App &app, Shapes &shapes, ArcShape &arc_shape, const Vec2 &P) {
	if (shapes.snap.in_distance && shapes.snap.is_node_shape) {
		if (shapes.snap.shape == SnapShape::LINE) {
			LineShape &line_shape = shapes.lines[shapes.snap.index];
			std::vector<Vec2> ixn_points = graphics::Arc2_Line2_intersect(arc_shape.arc, line_shape.line);
			set_snap_E(ixn_points, app, arc_shape);
		} else if (shapes.snap.shape == SnapShape::CIRCLE) {
			CircleShape &circle_shape = shapes.circles[shapes.snap.index];
			std::vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(arc_shape.arc, circle_shape.circle);
			set_snap_E(ixn_points, app, arc_shape);
		} else if (shapes.snap.shape == SnapShape::ARC) {
			ArcShape &arc_shape_2 = shapes.arcs[shapes.snap.index];
			std::vector<Vec2> ixn_points = graphics::Arc2_Arc2_intersect(arc_shape.arc, arc_shape_2.arc);
			set_snap_E(ixn_points, app, arc_shape);
		}
	} else {
		arc_shape.arc.E = circle2::project_point(arc_shape.arc.to_circle(), P);
	}
	arc_shape.arc.E_angle =
		circle2::get_angle_of_point(arc_shape.arc.to_circle(), arc_shape.arc.E);
}

void set_S(Shapes &shapes, ArcShape &arc_shape, const Vec2 &P) {
	if (shapes.ref.shape == RefShape::ARC) {
		double ref_radius = shapes.ref.value;
		assert(ref_radius >= 0);
		arc2::set_S(arc_shape.arc, ref_radius, P);
	} else {
		arc_shape.arc.S = P;
	}
	arc_shape.arc.S_angle = 
		circle2::get_angle_of_point(arc_shape.arc.to_circle(), arc_shape.arc.S);
}

void construct_arc(const App &app, Shapes &shapes, Vec2 const &P) {
	auto &arc_shape = shapes.construct.arc_shape;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::ARC;
			arc_shape.arc.C = P;
			arc_shape.concealed = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			set_S(shapes, arc_shape, P);
		} else if (shapes.construct.point_set == PointSet::SECOND) {
			set_E(app, shapes, arc_shape, P);
			arc_shape.concealed = construct.concealed;
			arc_shape.id = shapes.id_counter++;
			shapes.arcs.push_back(arc_shape);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		set_S(shapes, arc_shape, P);
		arc_shape.arc.S_angle = circle2::get_angle_of_point(arc_shape.arc.to_circle(), arc_shape.arc.S);
	} else if (shapes.construct.point_set == PointSet::SECOND) {
		set_E(app, shapes, arc_shape, P);
	}
}

void construct(const App &app, Shapes &shapes, Vec2 const &P) {
  switch (app.context.mode) {
    case AppMode::NORMAL:    return;
    case AppMode::LINE:      construct_line(app, shapes, P);   break;
    case AppMode::CIRCLE:    construct_circle(app, shapes, P); break;
    case AppMode::ARC:       construct_arc(app, shapes, P);    break;
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
