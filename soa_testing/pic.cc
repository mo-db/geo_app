#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <numbers>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>

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

constexpr uint32_t fg_color = white;
constexpr uint32_t bg_color = black;
constexpr uint32_t conceal_color = dark_grey;
constexpr uint32_t select_color = red;

constexpr uint32_t hl_primary_color = green;
constexpr uint32_t hl_secondary_color = cyan;
constexpr uint32_t hl_tertiary_color = magenta;

constexpr uint32_t special_color = blue;

namespace gk {
constexpr double epsilon = 1e-6;
constexpr double int_epsilon = 0.5;
} // namespace gk

namespace util {
inline void toggle_bool(bool &b) {
	b = b ? false : true;
}
inline bool epsilon_equal(double x, double y) {
	return (x < y + gk::epsilon && x > y - gk::epsilon);
}
} // namespace util

struct Vec2 {
  double x{0.}, y{0.};
  Vec2() = default;
  Vec2(const double x, const double y) : x{x}, y{y} {}
  // magnitude
  double mag() { return std::sqrt(x * x + y * y); }
  // normalize
  Vec2 norm() {
    double m = mag();
    return {x / m, y / m};
  }
};

Vec2 operator+(const Vec2 &a, const Vec2 &b) {
	return {a.x + b.x, a.y + b.y};
}
Vec2 operator-(const Vec2 &a, const Vec2 &b) {
	return {a.x - b.x, a.y - b.y};
}
Vec2 operator*(const Vec2 &v, double d) {
	return {v.x * d, v.y * d};
}
Vec2 operator*(double d, const Vec2 &v) {
	return v * d;
}

namespace vec2 {
double dot(const Vec2 &a, const Vec2 &b) {
	return a.x * b.x + a.y * b.y;
}
double distance(const Vec2 &a, const Vec2 &b) {
  return std::sqrt(std::pow(a.x - b.x, 2.0) + std::pow(a.y - b.y, 2.0));
}
bool equal_int_epsilon(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::int_epsilon &&
         std::abs(a.y - b.y) < gk::int_epsilon;
}
bool equal_epsilon(const Vec2 &a, const Vec2 &b) {
  return std::abs(a.x - b.x) < gk::epsilon &&
				 std::abs(a.y - b.y) < gk::epsilon;
}
// double get_angle(Vec2 P, Vec2 Q) {
	// Vec2 v = Q - P;
  // double angle = -std::atan2(v.y, v.x); // because 0/0 is up left
	// if (angle < 0) { angle += 2 * std::numbers::pi; }
	// return angle;
// }
} // namespace vec2

struct Line2 {
	Vec2 A{}, B{};
	Line2() = default;
	Line2(const Vec2 A, const Vec2 B) : A{A}, B{B} {}
	Vec2 get_a() const { return Vec2 {B.y - A.y, -(B.x - A.x)}; }
	Vec2 get_v() const { return Vec2 {B.x - A.x, B.y - A.y}; }
	double length() const { return vec2::distance(A, B); }
	Vec2 direction() const { return (B - A).norm(); }
};

struct TemporaryFlags {
	bool selected{false};
	bool hl_primary{false};
	bool hl_secondary{false};
	bool hl_tertiary{false};
};

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

struct Line: Shape {
	Line2 geom{};
	Line() = default;
  Line(int id, const Vec2 &A, const Vec2 &B)
		: Shape{id}, geom{A, B} {}
};

constexpr int image_width = 1024;
constexpr int image_height = 1024;
constexpr int n_pixels = image_width * image_height;
uint32_t pixel_buf[n_pixels];

void set_pixel(uint32_t *pixel_buf, int x, int y, uint32_t color) {
	if (x >= 0 && y >= 0 && x < image_width && y < image_height) {
		pixel_buf[x + y * image_width] = color;
	}
}

void plot_line(uint32_t *pixel_buf, const Line2 &line, uint32_t color) {
	int x0 = std::round(line.A.x);
	int y0 = std::round(line.A.y);
	int x1 = std::round(line.B.x);
	int y1 = std::round(line.B.y);

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  for (;;) { /* loop */
    set_pixel(pixel_buf, x0, y0, color);
    e2 = 2 * err;
    if (e2 >= dy) { /* e_xy+e_x > 0 */
      if (x0 == x1)
        break;
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) { /* e_xy+e_y < 0 */
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}


int main() {
  // Image


	// Random
	const int points_n = 100000;
	const int lines_n = points_n/2;

	std::vector<Vec2> points(points_n);
	// std::vector<Line2> lines(lines_n);
	std::vector<Line> shape_lines(lines_n);

	// Random number generator with nondeterministic seed
	std::random_device rd;
	std::mt19937 gen(rd());                  // Mersenne Twister engine
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	for (int i = 0; i < points_n; i++) {
			points.at(i).x = dist(gen) * image_width;
			points.at(i).y = dist(gen) * image_height;
	}

	int i = 0;
	int j = 0;
	for (;i < lines_n; i++, j+=2) {
			shape_lines.at(i).geom.A = points.at(j);
			shape_lines.at(i).geom.B = points.at(j+1);
	}

  // Render
	std::string save_file = "new_pic.ppm";
	std::ofstream save_out(save_file);
	assert(save_out);

  save_out << "P3\n" << image_width << ' ' << image_height << "\n255\n";

  auto t1 = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < n_pixels; i++) {
		pixel_buf[i] = bg_color;
	}
	for (auto &line : shape_lines) {
		plot_line(pixel_buf, line.geom, fg_color);
	}

  for (int j = 0; j < image_height; j++) {
    for (int i = 0; i < image_width; i++) {
      // auto r = double(i) / (image_width - 1);
      // auto g = double(j) / (image_height - 1);
      // auto b = 0.0;

      // int ir = int(255.999 * r);
      // int ig = int(255.999 * g);
      // int ib = int(255.999 * b);


			uint32_t pixel = pixel_buf[i + (j * image_width)];  // example pixel

			uint8_t ir = (pixel >> 16) & 0xFF;
			uint8_t ig = (pixel >> 8)  & 0xFF;
			uint8_t ib =  pixel        & 0xFF;

			save_out 
				<< int(ir) << ' '
				<< int(ig) << ' '
				<< int(ib) << '\n';

      // save_out << ir << ' ' << ig << ' ' << ib << '\n';
    }
  }
	save_out << std::endl;

  auto t2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> dt_ms = t2 - t1;
  std::cout << "dt_out: " << dt_ms.count() << std::endl;

	return 0;
}
