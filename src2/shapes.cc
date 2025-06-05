#include "shapes.hpp"

Line *Shapes::get_line_by_id(const int id) {
	for (auto &line : lines) {
		if (id == line.id) {
			return &line;
		}
	}
	return nullptr;
}
Circle *Shapes::get_circle_by_id(const int id) {
	for (auto &circle : circles) {
		if (id == circle.id) {
			return &circle;
		}
	}
	return nullptr;
}
Arc *Shapes::get_arc_by_id(const int id) {
	for (auto &arc : arcs) {
		if (id == arc.id) {
			return &arc;
		}
	}
	return nullptr;
}

namespace shapes {

void construct_line(const App &app, Shapes &shapes, const Vec2 &P) {
	auto &line = shapes.construct.line;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::LINE;
			line.geom.A = P;
			line.concealed  = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			if (shapes.ref.shape == RefShape::LINE) {
				double ref_length = shapes.ref.value;
				assert(ref_length >= 0);
				Vec2 v_norm = Line2{line.geom.A, P}.get_v().norm();
				line.geom.B = line.geom.B + v_norm * ref_length;
			} else {
				line.geom.B = P;
			}
			line.concealed = construct.concealed;
			line.id = shapes.id_counter++;
			shapes.lines.push_back(line);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (construct.point_set == PointSet::FIRST) {
		if (shapes.ref.shape == RefShape::LINE) {
			double ref_length = shapes.ref.value;
			assert(ref_length >= 0);
			Vec2 v_norm = Line2{line.geom.A, P}.get_v().norm();
			line.geom.B = line.geom.B + v_norm * ref_length;
		} else {
			line.geom.B = P;
		}
		line.geom.B = P;
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
	auto &circle = shapes.construct.circle;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::CIRCLE;
			circle.geom.C = P;
			circle.concealed = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			set_P(shapes, circle.geom, P);

			circle.concealed = construct.concealed;
			circle.id = shapes.id_counter++;
			shapes.circles.push_back(circle);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		set_P(shapes, circle.geom, P);
	}
}

// arc
void set_snap_E(std::vector<Vec2> ixn_points, const App &app, Arc &arc) {
	if (ixn_points.size() > 1) {
		if (vec2::distance(ixn_points.at(0), app.input.mouse) < 
				vec2::distance(ixn_points.at(1), app.input.mouse)) {
			arc.geom.E = ixn_points.at(0);
		} else {
			arc.geom.E = ixn_points.at(1);
		}
	} else {
		arc.geom.E = ixn_points.back();
	} 
}

void set_E(const App &app, Shapes &shapes, Arc &arc, const Vec2 &P) {
	if (shapes.snap.in_distance && shapes.snap.is_node_shape) {
		if (shapes.snap.shape == SnapShape::LINE) {
			Line &line = shapes.lines[shapes.snap.index];
			std::vector<Vec2> ixn_points = graphics::Arc2_Line2_intersect(arc.geom, line.geom);
			set_snap_E(ixn_points, app, arc);
		} else if (shapes.snap.shape == SnapShape::CIRCLE) {
			Circle &circle = shapes.circles[shapes.snap.index];
			std::vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(arc.geom, circle.geom);
			set_snap_E(ixn_points, app, arc);
		} else if (shapes.snap.shape == SnapShape::ARC) {
			Arc &arc_2 = shapes.arcs[shapes.snap.index];
			std::vector<Vec2> ixn_points = graphics::Arc2_Arc2_intersect(arc.geom, arc_2.geom);
			set_snap_E(ixn_points, app, arc);
		}
	} else {
		arc.geom.E = circle2::project_point(arc.geom.to_circle(), P);
	}
	arc.geom.E_angle =
		circle2::get_angle_of_point(arc.geom.to_circle(), arc.geom.E);
}

void set_S(Shapes &shapes, Arc &arc, const Vec2 &P) {
	if (shapes.ref.shape == RefShape::ARC) {
		double ref_radius = shapes.ref.value;
		assert(ref_radius >= 0);
		arc2::set_S(arc.geom, ref_radius, P);
	} else {
		arc.geom.S = P;
	}
	arc.geom.S_angle = 
		circle2::get_angle_of_point(arc.geom.to_circle(), arc.geom.S);
}

void construct_arc(const App &app, Shapes &shapes, Vec2 const &P) {
	auto &arc = shapes.construct.arc;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::ARC;
			arc.geom.C = P;
			arc.concealed = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			set_S(shapes, arc, P);
		} else if (shapes.construct.point_set == PointSet::SECOND) {
			set_E(app, shapes, arc, P);
			arc.concealed = construct.concealed;
			arc.id = shapes.id_counter++;
			shapes.arcs.push_back(arc);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		set_S(shapes, arc, P);
		arc.geom.S_angle = circle2::get_angle_of_point(arc.geom.to_circle(), arc.geom.S);
	} else if (shapes.construct.point_set == PointSet::SECOND) {
		set_E(app, shapes, arc, P);
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
		Line &line = shapes.lines[index];

		if (line2::get_distance_point_to_seg(line.geom, app.input.mouse) < snap.distance) {
			Vec2 projected_point = line2::project_point(line.geom, app.input.mouse);
			if (line2::point_in_segment_bounds(line.geom, projected_point)) {
				snap.point = projected_point;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				return true;
			} else if (vec2::distance(app.input.mouse, line.geom.A) < snap.distance) {
				snap.point = line.geom.A;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				return true;
			} else if (vec2::distance(app.input.mouse, line.geom.B) < snap.distance) {
				snap.point = line.geom.B;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				return true;
			}
		}
	}

	for (size_t index = 0; index < shapes.circles.size(); index++) {
		Circle &circle = shapes.circles[index];
		double distance = vec2::distance(circle.geom.C, app.input.mouse);
		if (distance < circle.geom.radius() + snap.distance &&
				distance > circle.geom.radius() - snap.distance) {
			snap.point = circle2::project_point(circle.geom, app.input.mouse);
			snap.shape = SnapShape::CIRCLE;
			snap.is_node_shape = false;
			snap.index = index;
			return true;
		}
	}

	for (size_t index = 0; index < shapes.arcs.size(); index++) {
		Arc &arc = shapes.arcs[index];
		double distance = vec2::distance(arc.geom.C, app.input.mouse);
		if (distance < arc.geom.radius() + snap.distance &&
				distance > arc.geom.radius() - snap.distance) {
			if (arc2::angle_on_arc(arc.geom, circle2::get_angle_of_point(
					arc.geom.to_circle(), app.input.mouse))) {
			snap.point = circle2::project_point(arc.geom.to_circle(), app.input.mouse);
			snap.shape = SnapShape::ARC;
			snap.is_node_shape = false;
			snap.index = index;
			return true;
			}
		}
	}
	return false;
}

void maybe_append_node(std::vector<Node> &node_shapes, Vec2 &P,
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
		node_shapes.push_back(Node{shape_id, P});
		if (point_concealed) {
			node_shapes.back().concealed = true;
		}
		cout << "node added" << endl; // NOTE
  }
}

void clear_edit(Shapes &shapes) {
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
		P = line2::project_point(shapes.edit.line.geom, shapes.snap.point);
	} else {
		P = line2::project_point(shapes.edit.line.geom, app.input.mouse);
	}
	if (vec2::distance(shapes.edit.line.geom.A, app.input.mouse) <
			vec2::distance(shapes.edit.line.geom.B, app.input.mouse)) {
		shapes.edit.line.geom.A = P;
	} else {
		shapes.edit.line.geom.B = P;
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
				shapes.circles.push_back(shapes.edit.circle);
				shapes.quantity_change = true;
				shapes.edit.in_edit = false;
			}
			if (shapes.edit.shape == EditShape::ARC) {
				arc_edit_update(app, shapes);
				shapes.arcs.push_back(shapes.edit.arc);
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
