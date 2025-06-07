// gen.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"
#include "app.hpp"
#include "shapes.hpp"

struct GenLine {
	Line shape;
	Vec2 start_point {};
	Vec2 dir_point {};
};
struct GenCircle {
	Circle shape;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenArc {
	Arc shape;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenShapes {
	std::vector<GenLine> lines;
	std::vector<GenCircle> circles;
	std::vector<GenArc> arcs;

	bool origin_set = false;
	Node origin{};
};

namespace gen {
namespace detail {
template <typename GenT, typename ShapeT>
bool shape_is_duplicate(const std::vector<GenT> &gen_shapes,
                        const ShapeT &shape) {
  return std::any_of(gen_shapes.begin(), gen_shapes.end(),
                     [shape](const GenT &gen_shape) {
                       return gen_shape.shape.id == shape.id;
                     });
}
void hl_nodes_of_shape(Shapes &shapes, int shape_id);
void hl_gen_shapes_and_nodes(Shapes &shapes, GenShapes &gen_shapes);
void set_origin(Shapes &shapes, GenShapes &gen_shapes, Node &node);
} // namespace detail
void maybe_select(Shapes &shapes, GenShapes &gen_shapes);

std::vector<double> line_relations(App &app, Shapes &shapes,
                                   GenLine &gen);
std::vector<double> circle_relations(App &app, Shapes &shapes,
                                     GenCircle &gen);
std::vector<double> arc_relations(App &app, Shapes &shapes,
                                  GenArc &gen);
// reset origin and others
void reset(Shapes &shapes, GenShapes &gen_shapes);
// clear gen shapes
// void clear(Shapes &shapes, GenShapes &gen_shapes);
} // namespace gen
