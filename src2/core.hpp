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

namespace util {
inline void toggle_bool(bool &b) {
	b = b ? false : true;
}
} // namespace util

namespace gk {
constexpr double epsilon = 1e-6;
constexpr double int_epsilon = 0.5;
} // namespace gk
