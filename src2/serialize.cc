#include "serialize.hpp"

namespace serialize {
void detail::serialize_line(const Line &line, ofstream &save_out) {
	save_out << line.geom.A.x << " " << line.geom.A.y << " " 
			<< line.geom.B.x << " " << line.geom.B.y << " "
			<< static_cast<int>(line.pflags.concealed) << " ";
}
Line detail::deserialize_line(ifstream &save_in) {
	Line line;
	int concealed_int {};
	save_in >> line.geom.A.x >> line.geom.A.y >> line.geom.B.x >> line.geom.B.y
		 >> concealed_int;
	line.pflags.concealed = static_cast<bool>(concealed_int);
	return line;
}

void detail::serialize_circle(const Circle &circle, ofstream &save_out) {
	save_out << circle.geom.C.x << " " << circle.geom.C.y << " "
			<< circle.geom.P.x << " " << circle.geom.P.y << " "
			<< static_cast<int>(circle.pflags.concealed) << " ";
}
Circle detail::deserialize_circle(ifstream &save_in) {
	Circle circle;
	int concealed_int {};
	save_in >> circle.geom.C.x >> circle.geom.C.y
		 >> circle.geom.P.x >> circle.geom.P.y >> concealed_int;
	circle.pflags.concealed = static_cast<bool>(concealed_int);
	return circle;
}

void detail::serialize_arc(const Arc &arc, ofstream &out) {
	out << arc.geom.C.x << " " << arc.geom.C.y << " "
			<< arc.geom.S.x << " " << arc.geom.S.y << " "
			<< arc.geom.E.x << " " << arc.geom.E.y << " "
			<< static_cast<int>(arc.pflags.concealed) << " ";
}
Arc detail::deserialize_arc(ifstream &in) {
	Arc arc;
	int concealed_int {};
	in >> arc.geom.C.x >> arc.geom.C.y
		 >> arc.geom.S.x >> arc.geom.S.y
		 >> arc.geom.E.x >> arc.geom.E.y >> concealed_int;
	arc.pflags.concealed = static_cast<bool>(concealed_int);

	arc.geom.S_angle =
			circle2::get_angle_of_point(arc.geom.to_circle(), arc.geom.S);
	arc.geom.E_angle =
			circle2::get_angle_of_point(arc.geom.to_circle(), arc.geom.E);
	return arc;
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
