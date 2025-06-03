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

bool update_snap(const App &app, Shapes &shapes) {
	auto &snap = shapes.snap;
	snap.index = -1;
	snap.point = {};
	snap.in_distance = false;
	snap.is_node_shape = false;
	snap.shape = SnapShape::NONE;

	if (snap.enabled_for_node_shapes) {
		for (size_t index = 0; index < shapes.ixn_points.size(); index++) {
			if (vec2::distance(shapes.ixn_points[index].P, app.input.mouse) < snap.distance) {
				snap.point = shapes.ixn_points[index].P;
				snap.shape = SnapShape::IXN_POINT;
				snap.is_node_shape = true;
				snap.index = index;
				return true;
			}	
		}

		for (size_t index = 0; index < shapes.def_points.size(); index++) {
			if (vec2::distance(shapes.def_points[index].P, app.input.mouse) < snap.distance) {
				snap.point = shapes.def_points[index].P;
				snap.shape = SnapShape::DEF_POINT;
				snap.is_node_shape = true;
				snap.index = index;
				return true;
			}	
		}
	}

	for (size_t index = 0; index < shapes.lines.size(); index++) {
		LineShape &line_shape = shapes.lines[index];
		if (line2::get_distance_point_to_seg(line_shape.line, app.input.mouse) < snap.distance) {
			Vec2 projected_point = line2::project_point(line_shape.line, app.input.mouse);
			double p1_distance = vec2::distance(app.input.mouse, line_shape.line.A);
			double p2_distance = vec2::distance(app.input.mouse, line_shape.line.B);
			if (line2::point_in_segment_bounds(line_shape.line, projected_point)) {
				snap.point = projected_point;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				return true;
			} else if (p1_distance < snap.distance) {
				snap.point = line_shape.line.A;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				return true;
			} else if (p2_distance < snap.distance) {
				snap.point = line_shape.line.B;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				return true;
			}
		}
	}

	for (size_t index = 0; index < shapes.circles.size(); index++) {
		CircleShape &circle_shape = shapes.circles[index];
		double distance = vec2::distance(circle_shape.circle.C, app.input.mouse);
		if (distance < circle_shape.circle.radius() + snap.distance &&
				distance > circle_shape.circle.radius() - snap.distance) {
			snap.point = circle2::project_point(circle_shape.circle, app.input.mouse);
			snap.shape = SnapShape::CIRCLE;
			snap.is_node_shape = false;
			snap.index = index;
			return true;
		}
	}

	for (size_t index = 0; index < shapes.arcs.size(); index++) {
		ArcShape &arc_shape = shapes.arcs[index];
		double distance = vec2::distance(arc_shape.arc.C, app.input.mouse);
		if (distance < arc_shape.arc.radius() + snap.distance &&
				distance > arc_shape.arc.radius() - snap.distance) {
			if (arc2::angle_on_arc(circle2::get_angle_of_point(
					arc_shape.arc.to_circle(), app.input.mouse))) {
			snap.point = circle2::project_point(arc_shape.arc.to_circle(), app.input.mouse);
			snap.shape = SnapShape::ARC;
			snap.is_node_shape = false;
			snap.index = index;
			return true;
			}
		}
	}
	return false;
}

void maybe_append_node(std::vector<NodeShape> &node_shapes, Vec2 &P,
                           int shape_id, bool point_concealed) {
  bool point_dup = false;
  for (auto &node_shape: node_shapes) {
		if (vec2::equal_int_epsilon(node_shape.P, P)) {
      point_dup = true;
      for (auto &id : node_shape.ids) {
				if (shape_id == id) {
					return;
        }
      }
			// add id to id point, maybe change conceal status of id point
			if (node_shape.concealed && !point_concealed) {
				node_shape.concealed = false;
			}
      node_shape.ids.push_back(shape_id);
			return;
    }
  }
	if (!point_dup) {
		// if new point push back and maybe flag as concealed
		node_shapes.push_back(NodeShape{shape_id, P});
		if (point_concealed) {
			node_shapes.back().concealed = true;
		}
  }
}

void clear_edit(const App &app, Shapes &shapes) {
	if (shapes.edit.in_edit) {
		if (shapes.edit.shape == EditShape::LINE) {
			shapes.lines.push_back(shapes.edit.line);
		} else if (shapes.edit.shape == EditShape::CIRCLE) {
			shapes.circles.push_back(shapes.edit.circle);
		} else if (shapes.edit.shape == EditShape::ARC) {
			shapes.arcs.push_back(shapes.edit.arc);
		}
	}
	shapes.edit.in_edit = false;
	shapes.snap.enabled_for_node_shapes = false;
}

void line_edit_update(const App &app, Shapes &shapes) {
	Vec2 P{};
	if (shapes.snap.in_distance) {
		P = line2::project_point(shapes.edit.line.line, shapes.snap.point);
	} else {
		P = line2::project_point(shapes.edit.line.line, app.input.mouse);
	}
	if (vec2::distance(shapes.edit.line.line.A, app.input.mouse) <
			vec2::distance(shapes.edit.line.line.B, app.input.mouse)) {
		shapes.edit.line.line.A = P;
	} else {
		shapes.edit.line.line.B = P;
	}
}

void circle_edit_update(const App &app, Shapes &shapes) {
}
void arc_edit_update(const App &app, Shapes &shapes) {
}

// maybe automatically disable node snap for first selecting 
// only works for lines at the moment
void update_edit(const App &app, Shapes &shapes) {
	if (!shapes.edit.in_edit) {
		if (app.input.mouse_click) {
			if (shapes.snap.shape == SnapShape::LINE) {
				shapes.edit.line = shapes.lines[shapes.snap.index];
				shapes.edit.in_edit = true;
				shapes.lines.erase(shapes.lines.begin() + shapes.snap.index);
			} else if (shapes.snap.shape == SnapShape::CIRCLE) {
				shapes.edit.circle = shapes.circles[shapes.snap.index];
				shapes.edit.in_edit = true;
				shapes.circles.erase(shapes.circles.begin() + shapes.snap.index);
			} else if (shapes.snap.shape == SnapShape::ARC) {
				shapes.edit.arc = shapes.arcs[shapes.snap.index];
				shapes.edit.in_edit = true;
				shapes.arcs.erase(shapes.arcs.begin() + shapes.snap.index);
			}
		} else {
			shapes.snap.enabled_for_node_shapes = false;
		}
	} else {
		shapes.snap.enabled_for_node_shapes = true;
		if (app.input.mouse_click) {
			if (shapes.edit.shape == EditShape::LINE) {
				line_edit_update(app, shapes);
				shapes.lines.push_back(shapes.edit.line);
				shapes.quantity_change = true;
				shapes.edit.in_edit = false;
			}
			if (shapes.edit.shape == EditShape::CIRCLE) {
				circle_edit_update(app, shapes);
				shapes.lines.push_back(shapes.edit.line);
				shapes.quantity_change = true;
				shapes.edit.in_edit = false;
			}
			if (shapes.edit.shape == EditShape::ARC) {
				arc_edit_update(app, shapes);
				shapes.lines.push_back(shapes.edit.line);
				shapes.quantity_change = true;
				shapes.edit.in_edit = false;
			}
		} else {
			if (shapes.edit.shape == EditShape::LINE) {
				line_edit_update(app, shapes);
			}
			if (shapes.edit.shape == EditShape::CIRCLE) {
				circle_edit_update(app, shapes);
			}
			if (shapes.edit.shape == EditShape::ARC) {
				arc_edit_update(app, shapes);
			}
		}
	}
}

} // namespace shapes
