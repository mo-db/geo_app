// core.hpp
#pragma once
#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <numbers>
#include <chrono>
#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>

using namespace std;

namespace gk {
constexpr double epsilon = 1e-6;
constexpr double int_epsilon = 0.5;
} // namespace gk

namespace color {
constexpr uint32_t black =				0x00000000;
constexpr uint32_t white =				0x00FFFFFF;
constexpr uint32_t dark_grey =		0x005C5C5C;
constexpr uint32_t light_grey =		0x00C5C5C5;

constexpr uint32_t red =					0x00FF0000;
constexpr uint32_t green =				0x0000FF00;
constexpr uint32_t blue =					0x000000FF;

constexpr uint32_t yellow =				0x00FFFF00;
constexpr uint32_t magenta =			0x00FF00FF;
constexpr uint32_t cyan =					0x0000FFFF;

constexpr uint32_t fg = white;
constexpr uint32_t bg = black;
constexpr uint32_t conceal = dark_grey;
constexpr uint32_t select = red;

constexpr uint32_t hl_primary = green;
constexpr uint32_t hl_secondary = cyan;
constexpr uint32_t hl_tertiary = magenta;

constexpr uint32_t special = blue;
} // namespace color

namespace util {
inline void toggle_bool(bool &b) {
	b = b ? false : true;
}
inline bool epsilon_equal(double x, double y) {
	return (x < y + gk::epsilon && x > y - gk::epsilon);
}
} // namespace util
