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

Line &Shapes::get_line_by_index(const size_t index) {
	assert(index <= lines.size());
	return lines[index];
}
Circle &Shapes::get_circle_by_index(const size_t index) {
	assert(index <= circles.size());
	return circles[index];
}
Arc &Shapes::get_arc_by_index(const size_t index) {
	assert(index <= arcs.size());
	return arcs[index];
}

namespace shapes {
void print_node_ids(Shapes &shapes) {
	if (shapes.snap.in_distance) {
		if (shapes.snap.shape == SnapShape::IXN_POINT) {
			Node ixn_point = shapes.ixn_points[shapes.snap.index];
			cout << "ixn_point: " << ixn_point.P.x << ", " << ixn_point.P.y << endl;
			cout << "ids: " << endl;
			for (auto &id : ixn_point.ids) {
				cout << id << ", ";
			}
			cout << endl;
		} else if (shapes.snap.shape == SnapShape::DEF_POINT) {
			Node def_point = shapes.def_points[shapes.snap.index];
			cout << "def_point: " << def_point.P.x << ", " << def_point.P.y << endl;
			cout << "ids: " << endl;
			for (auto &id : def_point.ids) {
				cout << id << ", ";
			}
			cout << endl;
		}
	}
}

void toggle_select(App &app, Shapes &shapes) {
  if (shapes.snap.in_distance && !shapes.snap.is_node_shape) {
    switch (shapes.snap.shape) {
    case SnapShape::LINE:
      util::toggle_bool(
          shapes.get_line_by_index(shapes.snap.index).tflags.selected);
      break;
    case SnapShape::CIRCLE:
      util::toggle_bool(
          shapes.get_circle_by_index(shapes.snap.index).tflags.selected);
      break;
    case SnapShape::ARC:
      util::toggle_bool(
          shapes.get_arc_by_index(shapes.snap.index).tflags.selected);
      break;
    default:
      exit(EXIT_FAILURE);
    }
  }
}

void pop_selected(Shapes &shapes) {
  shapes.lines.erase(
      remove_if(shapes.lines.begin(), shapes.lines.end(),
                [](const Line &line) { return line.tflags.selected; }),
      shapes.lines.end());
  shapes.circles.erase(
      remove_if(shapes.circles.begin(), shapes.circles.end(),
                [](const Circle &circle) { return circle.tflags.selected; }),
      shapes.circles.end());
  shapes.arcs.erase(
      remove_if(shapes.arcs.begin(), shapes.arcs.end(),
                [](const Arc &arc) { return arc.tflags.selected; }),
      shapes.arcs.end());
  shapes.quantity_change = true;
}

void pop_by_id(int id);

void construct_line(const App &app, Shapes &shapes, const Vec2 &P) {
	auto &line = shapes.construct.line;
	auto &construct = shapes.construct;

	if (app.input.mouse_click) {
		if (construct.point_set == PointSet::NONE) {
			construct.point_set = PointSet::FIRST;
			construct.shape = ConstructShape::LINE;
			line.geom.A = P;
			line.pflags.concealed  = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			if (shapes.ref.shape == RefShape::LINE) {
				double ref_length = shapes.ref.value;
				assert(ref_length >= 0);
				Vec2 v_norm = Line2{line.geom.A, P}.get_v().norm();
				line.geom.B = line.geom.A + v_norm * ref_length;
			} else {
				line.geom.B = P;
			}
			line.pflags.concealed = construct.concealed;
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
			circle.pflags.concealed = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			set_P(shapes, circle.geom, P);

			circle.pflags.concealed = construct.concealed;
			circle.id = shapes.id_counter++;
			shapes.circles.push_back(circle);
			shapes.quantity_change = true;
			construct.clear();
		}
	} else if (shapes.construct.point_set == PointSet::FIRST) {
		set_P(shapes, circle.geom, P);
	}
}

// arc NOTE use angle instead of distance to make better
void set_snap_E(std::vector<Vec2> ixn_points, const App &app, Arc &arc) {
	if (ixn_points.size() == 2) {
		if (vec2::distance(ixn_points.at(0), app.input.mouse) < 
				vec2::distance(ixn_points.at(1), app.input.mouse)) {
			arc.geom.E = ixn_points.at(0);
		} else {
			arc.geom.E = ixn_points.at(1);
		}
	} else if (ixn_points.size() == 1) {
		arc.geom.E = ixn_points.back();
	}
}

void set_E(const App &app, Shapes &shapes, Arc &arc, const Vec2 &P) {
	if (shapes.snap.in_distance && !shapes.snap.is_node_shape) {
		if (shapes.snap.shape == SnapShape::LINE) {
			Line &line = shapes.get_line_by_index(shapes.snap.index);
			std::vector<Vec2> ixn_points = graphics::Line2_Circle2_intersect(line.geom, arc.geom.to_circle());
			if (ixn_points.size() != 0) {
				set_snap_E(ixn_points, app, arc);
			} else {
				arc.geom.E = circle2::project_point(arc.geom.to_circle(), P);
			}
		} else if (shapes.snap.shape == SnapShape::CIRCLE) {
			Circle &circle = shapes.get_circle_by_index(shapes.snap.index);
			std::vector<Vec2> ixn_points = graphics::Circle2_Circle2_intersect(arc.geom.to_circle(), circle.geom);
			if (ixn_points.size() != 0) {
				set_snap_E(ixn_points, app, arc);
			} else {
				arc.geom.E = circle2::project_point(arc.geom.to_circle(), P);
			}
		} else if (shapes.snap.shape == SnapShape::ARC) {
			Arc &arc_2 = shapes.get_arc_by_index(shapes.snap.index);
			std::vector<Vec2> ixn_points = graphics::Arc2_Circle2_intersect(arc_2.geom, arc.geom.to_circle());
			if (ixn_points.size() != 0) {
				set_snap_E(ixn_points, app, arc);
			} else {
				arc.geom.E = circle2::project_point(arc.geom.to_circle(), P);
			}
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
			arc.pflags.concealed = construct.concealed;
		} else if (construct.point_set == PointSet::FIRST) {
			construct.point_set = PointSet::SECOND;
			set_S(shapes, arc, P);
			arc.pflags.concealed = construct.concealed;
		} else if (shapes.construct.point_set == PointSet::SECOND) {
			set_E(app, shapes, arc, P);
			arc.pflags.concealed = construct.concealed;
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
	snap.index = shapes.snap.index_unset;
	snap.id = -1;
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
				snap.id = shapes.ixn_points[index].id;
				return true;
			}	
		}

		for (size_t index = 0; index < shapes.def_points.size(); index++) {
			if (vec2::distance(shapes.def_points[index].P, app.input.mouse) < snap.distance) {
				snap.point = shapes.def_points[index].P;
				snap.shape = SnapShape::DEF_POINT;
				snap.is_node_shape = true;
				snap.index = index;
				snap.id = shapes.def_points[index].id;
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
				snap.id = shapes.lines[index].id;
				return true;
			} else if (vec2::distance(app.input.mouse, line.geom.A) < snap.distance) {
				snap.point = line.geom.A;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				snap.id = shapes.lines[index].id;
				return true;
			} else if (vec2::distance(app.input.mouse, line.geom.B) < snap.distance) {
				snap.point = line.geom.B;
				snap.shape = SnapShape::LINE;
				snap.is_node_shape = false;
				snap.index = index;
				snap.id = shapes.lines[index].id;
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
			snap.id = shapes.circles[index].id;
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
			snap.id = shapes.arcs[index].id;
			return true;
			}
		}
	}
	return false;
}

void maybe_append_node(std::vector<Node> &nodes, Vec2 &P,
                           int shape_id, bool node_concealed) {
  bool is_duplicate = false;
  for (auto &node : nodes) {
		if (vec2::equal_int_epsilon(node.P, P)) {
      is_duplicate = true;
			// test if id allready in node
      for (auto &id : node.ids) {
				if (shape_id == id) {
					return;
        }
      }
			// add id to id point, maybe change conceal status of id point
			if (node.pflags.concealed && !node_concealed) {
				node.pflags.concealed = false;
			}
      node.ids.push_back(shape_id);
			return;
    }
  }
	if (!is_duplicate) {
		// create the node
		nodes.push_back(Node{shape_id, P});
		// push back the shape id
		nodes.back().ids.push_back(shape_id);
		if (node_concealed) {
			nodes.back().pflags.concealed = true;
		}
  }
}

void maybe_select_ref(App &app, Shapes &shapes) {
	if (app.input.ctrl_set) {
		if (shapes.snap.shape == SnapShape::LINE) {
			auto &line = shapes.get_line_by_index(shapes.snap.index);
			if (shapes.ref.id == line.id) {
				shapes.ref.shape = RefShape::NONE;
			} else {
				shapes.ref.shape = RefShape::LINE;
				shapes.ref.value = line.geom.length();
				shapes.ref.id = line.id;
			}
		}
		if (shapes.snap.shape == SnapShape::CIRCLE) {
			auto &circle = shapes.get_circle_by_index(shapes.snap.index);
			if (shapes.ref.id == circle.id) {
				shapes.ref.shape = RefShape::NONE;
			} else {
				shapes.ref.shape = RefShape::CIRCLE;
				shapes.ref.value = circle.geom.radius();
				shapes.ref.id = circle.id;
			}
		}
		if (shapes.snap.shape == SnapShape::ARC) {
			auto &arc = shapes.get_arc_by_index(shapes.snap.index);
			if (shapes.ref.id == arc.id) {
				shapes.ref.shape = RefShape::NONE;
			} else {
				shapes.ref.shape = RefShape::ARC;
				shapes.ref.value = arc.geom.radius();
				shapes.ref.id = arc.id;
			}
		}
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
		shapes.snap.enabled_for_node_shapes = false;
		if (app.input.mouse_click) {
			if (shapes.snap.shape == SnapShape::LINE) {
				auto &line = shapes.get_line_by_index(shapes.snap.index);
				if (app.input.ctrl_set) {
					util::toggle_bool(line.pflags.concealed);
					shapes.quantity_change = true;
				} else {
					shapes.edit.line = line;
					shapes.edit.in_edit = true;
					shapes.edit.shape = EditShape::LINE;
					shapes.lines.erase(shapes.lines.begin() + shapes.snap.index);
				}
			} else if (shapes.snap.shape == SnapShape::CIRCLE) {
				auto &circle = shapes.get_circle_by_index(shapes.snap.index);
				if (app.input.ctrl_set) {
					assert(shapes.snap.index <= shapes.circles.size());
					util::toggle_bool(circle.pflags.concealed);
					shapes.quantity_change = true;
				} else {
					shapes.edit.circle = circle;
					shapes.edit.in_edit = true;
					shapes.edit.shape = EditShape::CIRCLE;
					shapes.circles.erase(shapes.circles.begin() + shapes.snap.index);
				}
			} else if (shapes.snap.shape == SnapShape::ARC) {
				auto &arc = shapes.get_arc_by_index(shapes.snap.index);
				if (app.input.ctrl_set) {
					util::toggle_bool(arc.pflags.concealed);
					shapes.quantity_change = true;
				} else {
					shapes.edit.arc = arc;
					shapes.edit.in_edit = true;
					shapes.edit.shape = EditShape::ARC;
					shapes.arcs.erase(shapes.arcs.begin() + shapes.snap.index);
				}
			}
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

void clear_tflags_global(Shapes &shapes) {
	for (auto &ixn_point: shapes.ixn_points) { ixn_point.clear_tflags(); }
	for (auto &def_point : shapes.def_points) { def_point.clear_tflags(); }
	for (auto &line: shapes.lines) { line.clear_tflags(); }
	for (auto &circle: shapes.circles) { circle.clear_tflags(); }
	for (auto &arc: shapes.arcs) { arc.clear_tflags(); }
}

void clear_tflags_hl_primary_global(Shapes &shapes) {
	for (auto &ixn_point: shapes.ixn_points) { ixn_point.tflags.hl_primary = false; }
	for (auto &def_point : shapes.def_points) { def_point.tflags.hl_primary = false; }
	for (auto &line: shapes.lines) { line.tflags.hl_primary = false; }
	for (auto &circle: shapes.circles) { circle.tflags.hl_primary = false; }
	for (auto &arc: shapes.arcs) { arc.tflags.hl_primary = false; }
}

void clear_tflags_hl_secondary_global(Shapes &shapes) {
	for (auto &ixn_point: shapes.ixn_points) { ixn_point.tflags.hl_secondary = false; }
	for (auto &def_point : shapes.def_points) { def_point.tflags.hl_secondary = false; }
	for (auto &line: shapes.lines) { line.tflags.hl_secondary = false; }
	for (auto &circle: shapes.circles) { circle.tflags.hl_secondary = false; }
	for (auto &arc: shapes.arcs) { arc.tflags.hl_secondary = false; }
}

void clear_tflags_hl_tertiary_global(Shapes &shapes) {
	for (auto &ixn_point: shapes.ixn_points) { ixn_point.tflags.hl_tertiary = false; }
	for (auto &def_point : shapes.def_points) { def_point.tflags.hl_tertiary = false; }
	for (auto &line: shapes.lines) { line.tflags.hl_tertiary = false; }
	for (auto &circle: shapes.circles) { circle.tflags.hl_tertiary = false; }
	for (auto &arc: shapes.arcs) { arc.tflags.hl_tertiary = false; }
}

bool id_match(const std::vector<int> &ids, const int shape_id) {
  return std::any_of(ids.begin(), ids.end(),
                     [shape_id](const int &id) { return shape_id == id; });
}
} // namespace shapes
