// serialize.hpp
#pragma once
#include "core.hpp"
#include "shapes.hpp"
#include "graphics.hpp"

namespace serialize {

namespace detail {
void serialize_line(const LineShape &line_shape, ofstream &out);
LineShape deserialize_line(ifstream &in);
void serialize_circle(const CircleShape &circle_shape, ofstream &out);
CircleShape deserialize_circle(ifstream &in);
void serialize_arc(const ArcShape &arc_shape, ofstream &out);
ArcShape deserialize_arc(ifstream &in);
} // namespace detail

void save_appstate(const Shapes &shapes, const std::string &save_file);
void load_appstate(Shapes &shapes, const std::string &save_file);
} // namespace serialize
