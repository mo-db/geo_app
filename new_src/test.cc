#include "save_sys.h"

int main() {
	olc::utils::DataFile df;
	auto &a = df["some_node"];
	a["name"].set_string("Moritz");
	a["age"].set_int(24);
	a["height"].set_real(1.78);

	auto &b = a["code"];
	b.set_string("c++", 0);
	b.set_string("vhdl", 1);

	return 0;
}
