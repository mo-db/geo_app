#include "gen.hpp"

namespace gen {


void hl_nodes_of_shape(Shapes &shapes, int shape_id) {
	for (auto &node : shapes.ixn_points) {
		if (!node.pflags.concealed && shapes::id_match(node.ids, shape_id)) {
			node.tflags.hl_secondary = true;
		}
	}
	for (auto &node : shapes.def_points) {
		if (!node.pflags.concealed && shapes::id_match(node.ids, shape_id)) {
			node.tflags.hl_secondary = true;
		}
	}
}

void hl_gen_shapes_and_nodes(Shapes &shapes, GenShapes &gen_shapes) {
	for (auto &line : gen_shapes.lines) {
		line.shape.tflags.hl_secondary = true;
		hl_nodes_of_shape(shapes, line.shape.id);
	}
	for (auto &circle : gen_shapes.circles) {
		circle.shape.tflags.hl_secondary = true;
		hl_nodes_of_shape(shapes, circle.shape.id);
	}
	for (auto &arc : gen_shapes.arcs) {
		arc.shape.tflags.hl_secondary = true;
		hl_nodes_of_shape(shapes, arc.shape.id);
	}
}

void reset(Shapes &shapes, GenShapes &gen_shapes) {
	shapes.snap.enabled_for_node_shapes = true;
	gen_shapes.origin_set = false;
	shapes::clear_tflags_hl_primary_global(shapes);
	hl_gen_shapes_and_nodes(shapes, gen_shapes);
}

void clear(Shapes &shapes, GenShapes &gen_shapes) {
	shapes.snap.enabled_for_node_shapes = true;
	gen_shapes.origin_set = false;
	gen_shapes.lines.clear();
	gen_shapes.circles.clear();
	gen_shapes.arcs.clear();
	gen_shapes.selection_order.clear();
}

void set_origin(Shapes &shapes, GenShapes &gen_shapes, Node &node) {
	// point origin to the node
	gen_shapes.origin = node;
	gen_shapes.origin_set = true;
	// disable snapping to nodes
	shapes.snap.enabled_for_node_shapes = false;
	// highlight all shapes that have their id in the origin node
	for (auto &line : shapes.lines) {
		line.tflags.hl_primary = shapes::id_match(gen_shapes.origin.ids, line.id);
	}
	for (auto &circle : shapes.circles) {
		circle.tflags.hl_primary = shapes::id_match(gen_shapes.origin.ids, circle.id);
	}
	for (auto &arc : shapes.arcs) {
		arc.tflags.hl_primary = shapes::id_match(gen_shapes.origin.ids, arc.id);
	}
	node.tflags.hl_primary = true;
}

void maybe_select(Shapes &shapes, GenShapes &gen_shapes) {
	if (gen_shapes.origin_set) {
		// if shape id is found in origin.ids and
		// if shape is not allready in a gen_shapes vector
		// add shape to a gen_shapes vector and update selection_order 
		if (shapes::id_match(gen_shapes.origin.ids, shapes.snap.id)) {
			if (shapes.snap.shape == SnapShape::LINE) {
				auto &line = shapes.lines[shapes.snap.index];
				if (detail::shape_is_duplicate(gen_shapes.lines, line)) {
					// do nothing
				} else {
					gen_shapes.lines.push_back(
						GenLine{line, gen_shapes.origin.P, shapes.snap.point});
					gen_shapes.selection_order.emplace_back(
							ShapeType::LINE, gen_shapes.lines.size() - 1);
					reset(shapes, gen_shapes);
					cout << "gen_line added" << endl;
				}
			} else if (shapes.snap.shape == SnapShape::CIRCLE) {
				auto &circle = shapes.circles[shapes.snap.index];
				if (detail::shape_is_duplicate(gen_shapes.circles, circle)) {
					// do nothing
				} else {
					gen_shapes.circles.push_back(
						GenCircle{circle, gen_shapes.origin.P, shapes.snap.point});
					gen_shapes.selection_order.emplace_back(
							ShapeType::CIRCLE, gen_shapes.circles.size() - 1);
					reset(shapes, gen_shapes);
					cout << "gen_circle added" << endl;
				}
			} else if (shapes.snap.shape == SnapShape::ARC) {
				auto &arc = shapes.arcs[shapes.snap.index];
				if (detail::shape_is_duplicate(gen_shapes.arcs, arc)) {
					// do nothing
				} else {
					gen_shapes.arcs.push_back(
						GenArc{arc, gen_shapes.origin.P, shapes.snap.point});
					gen_shapes.selection_order.emplace_back(
							ShapeType::ARC, gen_shapes.arcs.size() - 1);
					reset(shapes, gen_shapes);
					cout << "gen_arc added" << endl;
				}
			}
		}
	} else {
		if (shapes.snap.shape == SnapShape::IXN_POINT) {
			set_origin(shapes, gen_shapes, shapes.ixn_points[shapes.snap.index]);
		} else if (shapes.snap.shape == SnapShape::DEF_POINT) {
			set_origin(shapes, gen_shapes, shapes.def_points[shapes.snap.index]);
		}
	}
}

vector<double> line_relations(Shapes &shapes, GenLine &gen_line) {
	auto &line = gen_line.shape;

	vector<double> distances {};
	vector<double> distance_relations {};

	Vec2 A = gen_line.start_point;
	Vec2 B {};
	if (A.x == line.geom.A.x && A.y == line.geom.A.y) {
		B = line.geom.B;
	} else {
		B = line.geom.A;
	}

	double max_distance = (vec2::distance(A, B));

	// add distances between start_point and ixn_points to distances
	for (auto &ixn_point : shapes.ixn_points) {
		if (!ixn_point.pflags.concealed && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
										[&](const auto &id) { return id == line.id; })) {
			distances.push_back(vec2::distance(A, ixn_point.P));
		}
	}

	// add 0.0 to distances if not allready inside 
	auto iter = find_if(distances.begin(), distances.end(),
			[](double distance){ return fabs(distance) < gk::epsilon; });
	if (iter == distances.end()) {
		distances.push_back(0.0);
	}
	// remove max_distance from distances if not allready inside
	iter = find_if(distances.begin(), distances.end(),
			[=](double d){ return fabs(d - max_distance) < gk::epsilon; });
	if (iter != distances.end()) {
		distances.erase(iter);
	}

	// sort distances ascending
	sort(distances.begin(), distances.end(),
			 [](double v1, double v2) { return v1 < v2; });

	// normalize
	for (auto &distance : distances) {
		distance /= max_distance;
	}

#ifdef debug
	// info print
	cout << "distances: ";
	for (double distance : distances) {
			std::cout << distance << " ";
	}
	std::cout << std::endl;
	cout << "max distance: " << max_distance << endl;
	cout << "start point: " << A.x << "," << A.y << endl;
	cout << "end point: " << B.x << "," << B.y << endl;
#endif

	return distances;
}

// TODO: function should take some point on the circle as arg
vector<double> circle_relations(Shapes &shapes, GenCircle &gen_circle) {
	auto &circle = gen_circle.shape;
	vector<double> angles {};

	// put all angles of ixn_points into angles vector
	for (auto &ixn_point : shapes.ixn_points) {
		if (!ixn_point.pflags.concealed && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
										[&](const auto &id) { return id == circle.id; })) {
			angles.push_back(circle2::get_angle_of_point(circle.geom, ixn_point.P));
		}
	}

	// determine direction
	bool direction_clockwise = false;
	double start_angle = circle2::get_angle_of_point(circle.geom, gen_circle.start_point);
	double dir_angle = circle2::get_angle_of_point(circle.geom, gen_circle.dir_point);
	double start_angle_opposite {};
	if (start_angle > numbers::pi) {
		start_angle_opposite = start_angle - numbers::pi;
		if (dir_angle >= start_angle || dir_angle < start_angle_opposite) {
			direction_clockwise = false;
		} else {
			direction_clockwise = true;
		}
	} else {
		start_angle_opposite = start_angle + numbers::pi;
		if (dir_angle >= start_angle && dir_angle < start_angle_opposite) {
			direction_clockwise = false;
		} else {
			direction_clockwise = true;
		}
	}

	// apply offset rotation so that start_angle is 0.0
	for (auto &angle : angles) {
		double new_angle = angle - start_angle;
		if (abs(new_angle) > gk::epsilon) {
			if (new_angle < 0.0) {
				angle = 2 * numbers::pi + new_angle;
			} else {
				angle = new_angle;
			}
		} else {
			angle = 0.0;
		}
	}

	// flip angle values if clockwise
	if (direction_clockwise) {
		for (auto &angle : angles) {
			if (angle != 0.0) {
				angle = 2 * numbers::pi - angle;
			}
		}
	}

	// sort the vector of angle values 
	sort(angles.begin(), angles.end(),
			 [](double a1, double a2) { return a1 < a2; });

	// normalize
	for (auto &angle : angles) {
		if (angle > gk::epsilon) {
			angle = angle / (2.0 * numbers::pi);
		}
	}

	// info print
#ifdef debug
	std::cout << "Angles: ";
	for (double angle : angles) {
			std::cout << angle << " ";
	}
	std::cout << std::endl;
	std::cout << "start angle: " << start_angle << std::endl;
	std::cout << "dir angle: " << dir_angle << std::endl;
	cout << "clockwise: " << direction_clockwise << endl;
#endif

	return angles;
}

vector<double> arc_relations(Shapes &shapes, GenArc &gen) {
	// bool direction_clockwise = false;
	// vector<double> angles {};
	// vector<double> angle_relations {};

	// // fill all points into vector, if a point is selected put in front
	// for (auto &ixn_point : shapes.ixn_points) {
	// 	if (ixn_point.flags.highlighted && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
	// 									[&](const auto &id) { return id == gen.arc.id; })) {
	// 		angles.push_back(gen.arc.get_angle_of_point(ixn_point.p));
	// 	}
	// }

	// // determine direction
	// double start_angle = gen.arc.get_angle_of_point(gen.start_point);
	// double dir_angle = gen.arc.get_angle_of_point(gen.dir_point);
	// double start_angle_opposite {};
	// if (start_angle > numbers::pi) {
	// 	start_angle_opposite = start_angle - numbers::pi;
	// 	if (dir_angle >= start_angle || dir_angle < start_angle_opposite) {
	// 		// counter clockwise
	// 		direction_clockwise = false;
	// 		sort(angles.begin(), angles.end(),
	// 			[](double a1, double a2) { return a1 < a2; });
	// 	} else {
	// 		// clockwise
	// 		direction_clockwise = true;
	// 		sort(angles.begin(), angles.end(),
	// 			[](double a1, double a2) { return a1 > a2; });
	// 	}
	// } else {
	// 	start_angle_opposite = start_angle + numbers::pi;
	// 	if (dir_angle >= start_angle && dir_angle < start_angle_opposite) {
	// 		// counter clockwise
	// 		direction_clockwise = false;
	// 		sort(angles.begin(), angles.end(),
	// 			[](double a1, double a2) { return a1 < a2; });
	// 	} else {
	// 		// clockwise
	// 		direction_clockwise = true;
	// 		sort(angles.begin(), angles.end(),
	// 			[](double a1, double a2) { return a1 > a2; });
	// 	}
	// }

	// // Print the rotated vector
	// std::cout << "Sorted vector: ";
	// for (double angle : angles) {
	// 		std::cout << angle << " ";
	// }
	// std::cout << std::endl;

	// auto iter = find(angles.begin(), angles.end(), start_angle);
	// rotate(angles.begin(), iter, angles.end());

	// // Print the rotated vector
	// std::cout << "Rotated vector: ";
	// for (double angle : angles) {
	// 		std::cout << angle << " ";
	// }
	// std::cout << std::endl;

	// vector<double> final_angles {};
	// double final_angle {};

	// for (auto &angle : angles) {
	// 	if (direction_clockwise) {
	// 		if (angle <= angles.at(0)) {
	// 			final_angle = angles.at(0) - angle;
	// 		} else {
	// 			final_angle = (2 * numbers::pi - angle) + angles.at(0);
	// 		}
	// 	} else {
	// 		if (angle >= angles.at(0)) {
	// 			final_angle = angle - angles.at(0);
	// 		} else {
	// 			final_angle = (2 * numbers::pi + angle) - angles.at(0);
	// 		}
	// 	}
	// 	final_angle /= (2 * numbers::pi);
	// 	final_angles.push_back(final_angle);
	// }
	// return final_angles;
}

void calculate_relations(Shapes &shapes, GenShapes &gen_shapes,
                         std::ofstream &file_out) {
	// +1 after every shape, need refactor if i want to incoperate shape size
	int relation_depth = 0;
	std::vector<double> relations{};
	cout << "Relations: " << endl;
	for (auto [shape_type, index] : gen_shapes.selection_order) {
		switch (shape_type) {
			case ShapeType::LINE:
        assert(index < gen_shapes.lines.size());
				relations = gen::line_relations(shapes, gen_shapes.lines[index]);
				break;
			case ShapeType::CIRCLE:
        assert(index < gen_shapes.circles.size());
				relations = gen::circle_relations(shapes, gen_shapes.circles[index]);
				break;
			case ShapeType::ARC:
        assert(index < gen_shapes.arcs.size());
				relations = gen::arc_relations(shapes, gen_shapes.arcs[index]);
				break;
			default:
				std::exit(EXIT_FAILURE);
		}

		for (auto &relation : relations) {
			file_out << relation + relation_depth << " ";
			cout << relation + relation_depth << ", ";
		}
		cout << endl;

		relation_depth++;
	}
}

} // namespace gen
