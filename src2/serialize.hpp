// serialize.hpp
#pragma once
#include "core.hpp"
#include "shapes.hpp"
#include "graphics.hpp"

namespace serialize {

namespace detail {
void serialize_line(const Line &line_shape, ofstream &out);
Line deserialize_line(ifstream &in);
void serialize_circle(const Circle &circle_shape, ofstream &out);
Circle deserialize_circle(ifstream &in);
void serialize_arc(const Arc &arc_shape, ofstream &out);
Arc deserialize_arc(ifstream &in);
} // namespace detail

void save_appstate(const Shapes &shapes, const std::string &save_file);
void load_appstate(Shapes &shapes, const std::string &save_file);
} // namespace serialize
