#include "serialize.hpp"

namespace serialize {
void detail::serialize_line(const LineShape &line_shape, ofstream &save_out) {
	save_out << line_shape.line.A.x << " " << line_shape.line.A.y << " " 
			<< line_shape.line.B.x << " " << line_shape.line.B.y << " "
			<< static_cast<int>(line_shape.concealed) << " ";
}
LineShape detail::deserialize_line(ifstream &save_in) {
	LineShape line_shape;
	int concealed_int {};
	save_in >> line_shape.line.A.x >> line_shape.line.A.y >> line_shape.line.B.x >> line_shape.line.B.y
		 >> concealed_int;
	line_shape.concealed = static_cast<bool>(concealed_int);
	return line_shape;
}

void detail::serialize_circle(const CircleShape &circle_shape, ofstream &save_out) {
	save_out << circle_shape.circle.C.x << " " << circle_shape.circle.C.y << " "
			<< circle_shape.circle.P.x << " " << circle_shape.circle.P.y << " "
			<< static_cast<int>(circle_shape.concealed) << " ";
}
CircleShape detail::deserialize_circle(ifstream &save_in) {
	CircleShape circle_shape;
	int concealed_int {};
	save_in >> circle_shape.circle.C.x >> circle_shape.circle.C.y
		 >> circle_shape.circle.P.x >> circle_shape.circle.P.y >> concealed_int;
	circle_shape.concealed = static_cast<bool>(concealed_int);
	return circle_shape;
}

void detail::serialize_arc(const ArcShape &arc_shape, ofstream &out) {
	out << arc_shape.arc.C.x << " " << arc_shape.arc.C.y << " "
			<< arc_shape.arc.S.x << " " << arc_shape.arc.S.y << " "
			<< arc_shape.arc.E.x << " " << arc_shape.arc.E.y << " "
			<< static_cast<int>(arc_shape.concealed) << " ";
}
ArcShape detail::deserialize_arc(ifstream &in) {
	ArcShape arc_shape;
	int concealed_int {};
	in >> arc_shape.arc.C.x >> arc_shape.arc.C.y
		 >> arc_shape.arc.S.x >> arc_shape.arc.S.y
		 >> arc_shape.arc.E.x >> arc_shape.arc.E.y >> concealed_int;
	arc_shape.concealed = static_cast<bool>(concealed_int);

	arc_shape.arc.S_angle =
			circle2::get_angle_of_point(arc_shape.arc.to_circle(), arc_shape.arc.S);
	arc_shape.arc.E_angle =
			circle2::get_angle_of_point(arc_shape.arc.to_circle(), arc_shape.arc.E);
	return arc_shape;
}

void save_appstate(const Shapes &shapes, const std::string &save_file) {
	std::ofstream save_out(save_file);
	assert(save_out);

	save_out << shapes.lines.size() << " ";
	for (auto &line : shapes.lines) {
		detail::serialize_line(line, save_out);
	}
	save_out << endl;

	save_out << shapes.circles.size() << " ";
	for (auto &circle : shapes.circles) {
		detail::serialize_circle(circle, save_out);
	}
	save_out << endl;

	save_out << shapes.arcs.size() << " ";
	for (auto &arc : shapes.arcs) {
		detail::serialize_arc(arc, save_out);
	}
	save_out << endl;
}

// clear shapes and load saved ones
// the amout of shapes to load per line is determined by the start value
void load_appstate(Shapes &shapes, const std::string &save_file) {
	std::ifstream in(save_file);
	assert(in);
	shapes.quantity_change = true;

	shapes.lines.clear();
	size_t n_lines;
	in >> n_lines;
	for (size_t i = 0; i < n_lines; i++) {
	 shapes.lines.push_back(detail::deserialize_line(in));
	}

	shapes.circles.clear();
	size_t n_circles;
	in >> n_circles;
	for (size_t i = 0; i < n_circles; i++) {
		shapes.circles.push_back(detail::deserialize_circle(in));
	}

	shapes.arcs.clear();
	size_t n_arcs;
	in >> n_arcs;
	for (size_t i = 0; i < n_arcs; i++) {
		shapes.arcs.push_back(detail::deserialize_arc(in));
	}
}
} // namespace serialize
