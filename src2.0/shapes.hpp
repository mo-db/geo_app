// shapes.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"
#include "app.hpp"

struct Shape {
	int id{-1};
	bool selected{false};
	bool concealed{false};
	bool highlighted{false};
	Shape() = default;
	Shape(const int id) : id{id} {}
};

struct NodeShape : Shape {
	Vec2 P{};
	NodeShape() = default;
	NodeShape(const int id, const Vec2 &P)
		: Shape{id}, P{P} {}
};

struct LineShape : Shape {
	Line2 line{};
	LineShape() = default;
  LineShape(int id, const Vec2 &A, const Vec2 &B)
		: Shape{id}, line{A, B} {}
};

struct CircleShape : Shape {
	Circle2 circle;
	CircleShape() = default;
  CircleShape(int id, const Vec2 &C, const Vec2 &P)
		: Shape{id}, circle{C, P} {}
};

struct ArcShape : Shape {
	Arc2 arc;
	ArcShape() = default;
  ArcShape(int id, const Vec2 &C,
					 const Vec2 &S, const Vec2 &E)
		: Shape{id}, arc{C, S, E} {}
};

enum struct PointSet { NONE, FIRST, SECOND, };
enum struct ConstructShape{ NONE, LINE, CIRCLE, ARC, };
struct Construct {
	bool concealed = false;
	ConstructShape shape = ConstructShape::NONE;
	PointSet point_set = PointSet::NONE;

	LineShape line_shape {};
	CircleShape circle_shape {};
	ArcShape arc_shape {};

	void clear() {
		shape = ConstructShape::NONE;
		point_set = PointSet::NONE;
	}
};

enum struct EditShape { NONE, LINE, CIRCLE, ARC, };
struct Edit {
	EditShape shape = EditShape::NONE;
	LineShape line_shape {};
	CircleShape circle_shape {};
	ArcShape arc_shape {};
};

// can be selected in normal mode, used when modkey draw new shape
enum struct RefShape{ NONE, LINE, CIRCLE, ARC, };
struct Ref {
	RefShape shape = RefShape::NONE;
	int id {-1};
	double value {};
};

enum struct SnapShape { NONE, IXN_POINT, DEF_POINT, LINE, CIRCLE, ARC, };
struct Snap {
	const double distance = 20.0; // why cant i do constexpr?
	bool enabled_for_node_shapes = true;

	Vec2 point;
	int id {-1};
	int index {};
	bool is_node_shape = false;
	bool in_distance = false;
	SnapShape shape = SnapShape::NONE;
};

struct Shapes {
	std::vector<LineShape> lines;
	std::vector<CircleShape> circles;
	std::vector<ArcShape> arcs;
	std::vector<NodeShape> ixn_points;
	std::vector<NodeShape> def_points;

	uint32_t id_counter {};
	bool quantity_change = false;
	bool recalculate = false;

	Construct construct;
	Edit edit;
	Ref ref;
	Snap snap;

	LineShape *get_line_by_id(const int id);
	CircleShape *get_circle_by_id(const int id);
	ArcShape *get_arc_by_id(const int id);
};

namespace shapes {
// shapes general

void pop_selected(App &app);
void pop_by_id(int id);

// constructing shapes
namespace detail {
void construct_line(const App &app, Shapes &shapes, const Vec2 &pt);
void construct_circle(const App &app, Shapes &shapes, const Vec2 &pt);
void construct_arc(const App &app, Shapes &shapes, const Vec2 &pt);
} // namespace detail
void construct(App &, const Vec2 &point);

// editing shapes TODO


// shape reference TODO
// if in snap and click with special key held 
// copy (shape determined) parameter to ref_variable
// on drawing check if id = ref_id -> change color

// functions for snapping
bool update_snap(App &app);
} // namespace shapes
