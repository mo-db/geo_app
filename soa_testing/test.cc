#include <iostream>
#include <vector>
// #include <chrono>

struct Vec2 {
  float x{0.}, y{0.};
	// int foo = 5;
	// double bar = 99.99;
  Vec2() = default;
  Vec2(const float x, const float y) : x{x}, y{y} {}
};

int main() {
	int i_max = 0xFFFFFF;

	// SoA
	std::vector<float> x;
	std::vector<float> y;
	for (int i = 0; i < i_max; i++) {
		x.emplace_back(float(i) * 0.97F);
		y.emplace_back(float(i) * 0.59F);
	}

	// AoS
	std::vector<Vec2> v;
	for (int i = 0; i < i_max; i++) {
		v.emplace_back(Vec2{float(i) * 0.97F, float(i) * 0.59F});
	}

  auto start = std::chrono::high_resolution_clock::now();

	// for (auto &xval : x) {
	// 	xval *=0.8;
	// }

	// for (auto &vval: v) {
	// 	vval.x *=0.8;
	// }

	// for (int i = 0; i < i_max; i ++) {
	// 	x.at(i) *= 0.98;
	// 	y.at(i) *= 0.98;
	// 	y.at(i) *= x.at(i);
	// 	x.at(i) /= y.at(i);
	// }

	for (int i = 0; i < i_max; i ++) {
		v.at(i).x *= 0.98;
		v.at(i).y *= 0.98;
		v.at(i).y *= v.at(i).x;
		v.at(i).x /= v.at(i).y;
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> dt = end - start;
	std::cout << "Loop took " << dt.count() << " ms.\n";
	return 0;
}
