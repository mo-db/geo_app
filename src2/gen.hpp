#include "core.hpp"
#include "graphics.hpp"
#include "app.hpp"
#include "shapes.hpp"

struct GenLine {
	LineShape line_shape;
	Vec2 start_point {};
	Vec2 dir_point {};
};
struct GenCircle {
	CircleShape circle_shape;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenArc {
	ArcShape arc_shape;
	Vec2 start_point {};
	Vec2 dir_point {};
};

struct GenShapes {
	std::vector<GenLine> lines;
	std::vector<GenCircle> circles;
	std::vector<GenArc> arcs;

	bool start_set = false;
	NodeShape start_node {};
	std::vector<int> start_node_ids {};
};

namespace gen {

void toogle_hl_of_id_points(Shapes &shapes);
void hl_id_points_gen_select(Shapes &shapes, int shape_id);
void maybe_select(App &app, Shapes &shapes, GenShapes &gen_shapes);

std::vector<double> line_relations(const App &app, const Shapes &shapes,
                                   const GenLine &gen);
std::vector<double> circle_relations(const App &app, const Shapes &shapes,
                                     const GenCircle &gen);
std::vector<double> arc_relations(const App &app, const Shapes &shapes,
                                  const GenArc &gen);
} // namespace gen
