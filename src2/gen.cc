#include "gen.hpp"

namespace gen {
// if in Gen mode and ctrl click on id point -> toogle hl
void toogle_hl_of_id_points(Shapes &shapes) {
	// assert(shapes.snap_is_id_point);
	// if (shapes.snap_shape == SnapShape::IXN_POINT) {
	// 	bool &highlighted = shapes.ixn_points[shapes.snap_index].flags.highlighted;
	// 	if (highlighted) {
	// 		highlighted = false;
	// 	} else {
	// 		highlighted = true;
	// 	}
	// } else if (shapes.snap_shape == SnapShape::DEF_POINT) {
	// 	bool &highlighted = shapes.def_points[shapes.snap_index].flags.highlighted;
	// 	if (highlighted) {
	// 		highlighted = false;
	// 	} else {
	// 		highlighted = true;
	// 	}
	// }
}

void hl_id_points_gen_select(Shapes &shapes, int shape_id) {
	// for (auto &ixn_point : shapes.ixn_points) {
	// 	if (!ixn_point.flags.concealed) {
	// 		for (auto &id : ixn_point.ids) {
	// 			if (id == shape_id) {
	// 				ixn_point.flags.highlighted = true;
	// 				cout << "point highlighted" << endl;
	// 				break;
	// 			}
	// 		}
	// 	}
	// }

	// for (auto &id_point : shapes.def_points) {
	// 	if (!id_point.flags.concealed) {
	// 		for (auto &id : id_point.ids) {
	// 			if (id == shape_id) {
	// 				cout << "point highlighted" << endl;
	// 				id_point.flags.highlighted = true;
	// 				break;
	// 			}
	// 		}
	// 	}
	// }
}

// TODO implement arc gen select
void maybe_select(App &app, Shapes &shapes, GenShapes &gen_shapes) {
  // if (shapes.snap_is_id_point) {
		// if (shapes.snap_shape == SnapShape::IXN_POINT) {
				// shapes.gen_start_point_ids = shapes.ixn_points[shapes.snap_index].ids;
		// } else if (shapes.snap_shape == SnapShape::DEF_POINT) {
				// shapes.gen_start_point_ids = shapes.def_points[shapes.snap_index].ids;
		// }
		// shapes.gen_start_point = shapes.snap_point;
		// shapes.gen_start_set = true;
  // } else if (!shapes.snap_is_id_point && shapes.gen_start_set) {

		// if (shapes.snap_shape == SnapShape::LINE) {
			// Line2 line = shapes.lines[shapes.snap_index];
			// for (auto &id : shapes.gen_start_point_ids) {
				// if (line.id == id) {
					// line.flags.highlighted = true;
					// gen_shapes.lines.push_back(
						// GenLine{line, shapes.gen_start_point, shapes.snap_point});
					// hl_id_points_gen_select(shapes, line.id);
					// gen_shapes.start_set = false;
					// break;
				// }
			// }
		// }

		// if (shapes.snap_shape == SnapShape::CIRCLE) {
			// Circle2 circle = shapes.circles[shapes.snap_index];
			// for (auto & id : shapes.gen_start_point_ids) {
				// if (circle.id == id) {
					// circle.flags.highlighted = true;
					// gen_shapes.circles.push_back(
						// GenCircle{circle, shapes.gen_start_point, shapes.snap_point});
					// hl_id_points_gen_select(shapes, circle.id);
					// gen_shapes.start_set = false;
					// break;
				// }
			// }
		// }

		// if (shapes.snap_shape == SnapShape::ARC) {
			// Arc2 arc = shapes.arcs[shapes.snap_index];
			// for (auto & id : shapes.gen_start_point_ids) {
				// if (arc.id == id) {
					// arc.flags.highlighted = true;
					// gen_shapes.arcs.push_back(
						// GenArc{arc, shapes.gen_start_point, shapes.snap_point});
					// hl_id_points_gen_select(shapes, arc.id);
					// gen_shapes.start_set = false;
					// break;
				// }
			// }
		// }
  // }
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
