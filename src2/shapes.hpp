// shapes.hpp
#pragma once
#include "core.hpp"
#include "graphics.hpp"
#include "app.hpp"

// these flags can be reset when changing modes or pressing escape
struct TemporaryFlags {
	bool selected{false};
	bool hl_primary{false};
	bool hl_secondary{false};
	bool hl_tertiary{false};
};

// these flags are guaranteed to be persitent when changing modes
struct PersistentFlags {
	bool concealed{false};
};

enum struct ShapeType { NONE, IXN_POINT, DEF_POINT, LINE, CIRCLE, ARC };
struct Shape {
	int id{-1};
	TemporaryFlags tflags;
	PersistentFlags pflags;
	Shape() = default;
	Shape(const int id) : id{id} {}
	void clear_tflags() {
		tflags.selected = false;
		tflags.hl_primary = false;
		tflags.hl_secondary = false;
		tflags.hl_tertiary = false;
	}
	void clear_pflags() {
		pflags.concealed = false;
	}
	void clear_hl() {
		tflags.hl_primary = false;
		tflags.hl_secondary = false;
		tflags.hl_tertiary = false;
	}
	bool highlighted() {
		return tflags.hl_primary || tflags.hl_secondary || tflags.hl_tertiary;
	}
};

struct Node: Shape {
	Vec2 P{};
	std::vector<int> ids;
	Node() = default;
	Node(const int id, const Vec2 &P)
		: Shape{id}, P{P} {}
};

struct Line: Shape {
	Line2 geom{};
	Line() = default;
  Line(int id, const Vec2 &A, const Vec2 &B)
		: Shape{id}, geom{A, B} {}
};

struct Circle : Shape {
	Circle2 geom;
	Circle() = default;
  Circle(int id, const Vec2 &C, const Vec2 &P)
		: Shape{id}, geom{C, P} {}
};

struct Arc : Shape {
	Arc2 geom;
	Arc() = default;
  Arc(int id, const Vec2 &C, const Vec2 &S, const Vec2 &E)
		: Shape{id}, geom{C, S, E} {}
};

enum struct PointSet { NONE, FIRST, SECOND, };
enum struct ConstructShape{ NONE, LINE, CIRCLE, ARC, };
struct Construct {
	bool concealed = false;
	ConstructShape shape = ConstructShape::NONE;
	PointSet point_set = PointSet::NONE;

	Line line {};
	Circle circle {};
	Arc arc {};

	void clear() {
		shape = ConstructShape::NONE;
		point_set = PointSet::NONE;
		concealed = false;
	}
};

enum struct EditShape { NONE, LINE, CIRCLE, ARC, };
struct Edit {
	EditShape shape = EditShape::NONE;
	bool in_edit = false;
	int id {-1};
	Line line {};
	Circle circle {};
	Arc arc {};
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
	static constexpr double distance = 20.0;
	bool enabled_for_node_shapes = true;

	static constexpr size_t index_unset = std::numeric_limits<size_t>::max();
	size_t index {index_unset};
	int id {-1};
	bool is_node_shape = false;
	bool in_distance = false;
	SnapShape shape = SnapShape::NONE;
	Vec2 point;
};

struct Shapes {
	std::vector<Line> lines;
	std::vector<Circle> circles;
	std::vector<Arc> arcs;
	std::vector<Node> ixn_points;
	std::vector<Node> def_points;

	uint32_t id_counter {};
	bool quantity_change = false;
	bool recalculate = false;

	Construct construct;
	Edit edit;
	Ref ref;
	Snap snap;

	Line *get_line_by_id(const int id);
	Circle *get_circle_by_id(const int id);
	Arc *get_arc_by_id(const int id);

	Line &get_line_by_index(const size_t index);
	Circle &get_circle_by_index(const size_t index);
	Arc &get_arc_by_index(const size_t index);
};

namespace shapes {
// shapes general

void pop_selected(Shapes &shapes);
// void pop_by_id(int id);
void toggle_select(App &app, Shapes &shapes);
void print_node_ids(Shapes &shapes);
// constructing shapes
namespace detail {
void construct_line(const App &app, Shapes &shapes, const Vec2 &pt);
void construct_circle(const App &app, Shapes &shapes, const Vec2 &pt);
void construct_arc(const App &app, Shapes &shapes, const Vec2 &pt);
} // namespace detail
void construct(const App &app, Shapes &shapes, const Vec2 &point);

// editing shapes
void clear_edit(Shapes &shapes);
namespace detail {
void line_edit_update(const App &app, Shapes &shapes);
void circle_edit_update(const App &app, Shapes &shapes);
void arc_edit_update(const App &app, Shapes &shapes);
} // namespace detail
void update_edit(const App &app, Shapes &shapes);

// shape reference TODO
// if in snap and click with special key held 
// copy (shape determined) parameter to ref_variable
// on drawing check if id = ref_id -> change color
void maybe_select_ref(App &app, Shapes &shapes);

// functions for snapping
bool update_snap(const App &app, Shapes &shapes);
void maybe_append_node(std::vector<Node> &nodes, Vec2 &P,
                           int shape_id, bool point_concealed);
void clear_tflags_global(Shapes &shapes);
void clear_tflags_hl_primary_global(Shapes &shapes);
void clear_tflags_hl_secondary_global(Shapes &shapes);
void clear_tflags_hl_tertiary_global(Shapes &shapes);

bool id_match(const std::vector<int> &ids, const int shape_id);
} // namespace shapes
