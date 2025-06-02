#pragma once
#include "graphics.h"
#include <ostream>
#include <istream>
#include <iostream>

const std::string lines_save = "save";

namespace simseri {
	void save_appstate(const Shapes &shapes, const std::string &save_file);
	void load_appstate(const std::string &save_file);
	void serialize_line(const graphics::Line2 &line);
	void serialize_circle(const graphics::Circle2 &line);
	void serialize_arc(const graphics::Arc2 &line);
} // namespace simseri
