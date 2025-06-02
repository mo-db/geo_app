#include "save_sys.h"

int main() {
	olc::utils::DataFile df;
	auto &a = df["some_node"]; // return DataFile &
	a["name"].set_string("Moritz");
	a["age"].set_int(24);
	a["height"].set_real(1.78);

	auto &b = a["code"];
	b.set_string("c++", 0);
	b.set_string("vhdl", 1);

	// read and write functions are kept as static functions
	// belonging to the DataFile class -> to the namespace of that class
	olc::utils::DataFile::write(df, "TestOutput.txt");

	return 0;
}
