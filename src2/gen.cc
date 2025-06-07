#include "gen.hpp"

namespace gen {


void hl_nodes_of_shape(Shapes &shapes, int shape_id) {
	for (auto &node : shapes.ixn_points) {
		node.tflags.hl_secondary = shapes::id_match(node.ids, shape_id);
	}
	for (auto &node : shapes.def_points) {
		node.tflags.hl_secondary = shapes::id_match(node.ids, shape_id);
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

// TODO implement arc gen select
// first clear all hl, selection
// in edit mode if strg_held, toggle concealed of shape
// in draw mode press key to enable disable concealed drawing
//
// if origin set, disable snap to nodes
// need two flags/colors one to hl origin
// i need two selection colors?
//
// NOTE maybe change name to update select and handle deletion with ctrl
void maybe_select(Shapes &shapes, GenShapes &gen_shapes) {
	if (gen_shapes.origin_set) {
		if (shapes::id_match(gen_shapes.origin.ids, shapes.snap.id)) {
			if (shapes.snap.shape == SnapShape::LINE) {
				cout << "line" << endl;
				auto &line = shapes.lines[shapes.snap.index];
				if (detail::shape_is_duplicate(gen_shapes.lines, line)) {
					// do nothing
				} else {
					gen_shapes.lines.push_back(
						GenLine{line, gen_shapes.origin.P, shapes.snap.point});
					reset(shapes, gen_shapes);
				}
			} else if (shapes.snap.shape == SnapShape::CIRCLE) {
				cout << "circle" << endl;
				auto &circle = shapes.circles[shapes.snap.index];
				if (detail::shape_is_duplicate(gen_shapes.circles, circle)) {
					// do nothing
				} else {
					gen_shapes.circles.push_back(
						GenCircle{circle, gen_shapes.origin.P, shapes.snap.point});
					reset(shapes, gen_shapes);
				}
			} else if (shapes.snap.shape == SnapShape::ARC) {
				cout << "arc" << endl;
				auto &arc = shapes.arcs[shapes.snap.index];
				if (detail::shape_is_duplicate(gen_shapes.arcs, arc)) {
					// do nothing
				} else {
					gen_shapes.arcs.push_back(
						GenArc{arc, gen_shapes.origin.P, shapes.snap.point});
					reset(shapes, gen_shapes);
				}
			}
		}
	} else {
		if (shapes.snap.shape == SnapShape::IXN_POINT) {
			cout << "ixn" << endl;
			set_origin(shapes, gen_shapes, shapes.ixn_points[shapes.snap.index]);
		} else if (shapes.snap.shape == SnapShape::DEF_POINT) {
			cout << "def" << endl;
			set_origin(shapes, gen_shapes, shapes.def_points[shapes.snap.index]);
		}
	}
}


// TODO: endpoint mix with intersection points, leads to problems
vector<double> line_relations(App &app, Shapes &shapes,
                                         GenLine &gen) {
	// vector<double> distances {};
	// vector<double> distance_relations {};

	// Vec2 A = gen.start_point;
	// Vec2 B {};
	// if (A.x == gen.line.p1.x && A.y == gen.line.p1.y) {
	// 	B = gen.line.p2;
	// } else {
	// 	B = gen.line.p1;
	// }

	// double max_distance = (get_point_point_distance(A, B));

	// cout << "max distance: " << max_distance << endl;
	// cout << "start point: " << A.x << "," << A.y << endl;
	// cout << "end point: " << B.x << "," << B.y << endl;
	// cout << "p1 point: " << gen.line.p1.x << "," << gen.line.p1.y << endl;
	// cout << "p2 point: " << gen.line.p2.x << "," << gen.line.p2.y << endl;
	// cout << "dir point: " << gen.dir_point.x << "," << gen.dir_point .y << endl;

	// for (auto &ixn_point : shapes.ixn_points) {
	// 	if (ixn_point.flags.highlighted && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
	// 									[&](const auto &id) { return id == gen.line.id; })) {
	// 		distances.push_back(get_point_point_distance(A, ixn_point.p));
	// 	}
	// }

	// cout << "distances: ";
	// for (double distance : distances) {
	// 		std::cout << distance << " ";
	// }
	// std::cout << std::endl;

	// // push back start and end distance if not allready in 
	// auto iter = find_if(distances.begin(), distances.end(), 
	// 		[](double distance){ return fabs(distance - 0.0) < gk::epsilon; });
	// if (iter == distances.end()) {
	// 	distances.push_back(0.0);
	// }
	// iter = find_if(distances.begin(), distances.end(), 
	// 		[=](double d){ return fabs(d - max_distance) < gk::epsilon; });
	// if (iter != distances.end()) {
	// 	distances.erase(iter);
	// }

	// // sort ascending
	// sort(distances.begin(), distances.end(), [](double v1, double v2){ return v1 < v2; });

	// // normalize
	// for (auto &distance : distances) {
	// 	distance /= max_distance;
	// }
	// return distances;
}

// TODO: function should take some point on the circle as arg
vector<double> circle_relations(App &app, Shapes &shapes,
                                          GenCircle &gen) {
	// bool direction_clockwise = false;
	// vector<double> angles {};
	// vector<double> angle_relations {};

	// // fill all points into vector, if a point is selected put in front
	// for (auto &ixn_point : shapes.ixn_points) {
	// 	if (ixn_point.flags.highlighted && std::any_of(ixn_point.ids.begin(), ixn_point.ids.end(),
	// 									[&](const auto &id) { return id == gen.circle.id; })) {
	// 		angles.push_back(gen.circle.get_angle_of_point(ixn_point.p));
	// 	}
	// }

	// // determine direction
	// double start_angle = gen.circle.get_angle_of_point(gen.start_point);
	// double dir_angle = gen.circle.get_angle_of_point(gen.dir_point);
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

vector<double> arc_relations(App &app, Shapes &shapes,
                                          GenArc &gen) {
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

} // namespace gen
