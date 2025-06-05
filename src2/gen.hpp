// gen.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"
#include "app.hpp"
#include "shapes.hpp"

struct GenLine {
	Line line;
	Vec2 start_point {};
	Vec2 dir_point {};
};
struct GenCircle {
	Circle circle;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenArc {
	Arc arc;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenShapes {
	std::vector<GenLine> lines;
	std::vector<GenCircle> circles;
	std::vector<GenArc> arcs;

	bool start_set = false;
	Node start_node {};
	std::vector<int> start_node_ids {};
};

namespace gen {

void toogle_hl_of_id_points(Shapes &shapes);
void hl_id_points_gen_select(Shapes &shapes, int shape_id);
void maybe_select(App &app, Shapes &shapes, GenShapes &gen_shapes);

std::vector<double> line_relations(App &app, Shapes &shapes,
                                   GenLine &gen);
std::vector<double> circle_relations(App &app, Shapes &shapes,
                                     GenCircle &gen);
std::vector<double> arc_relations(App &app, Shapes &shapes,
                                  GenArc &gen);
} // namespace gen
