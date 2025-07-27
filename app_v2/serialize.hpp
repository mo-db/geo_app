// serialize.hpp
#pragma once
#include "core.hpp"
#include "shapes.hpp"
#include "graphics.hpp"

namespace serialize {

namespace detail {
void serialize_line(const Line &line_shape, std::ofstream &out);
Line deserialize_line(std::ifstream &in);
void serialize_circle(const Circle &circle_shape, std::ofstream &out);
Circle deserialize_circle(std::ifstream &in);
void serialize_arc(const Arc &arc_shape, std::ofstream &out);
Arc deserialize_arc(std::ifstream &in);
} // namespace detail

void save_appstate(const Shapes &shapes, const std::string &save_file);
void load_appstate(Shapes &shapes, const std::string &save_file);
} // namespace serialize
