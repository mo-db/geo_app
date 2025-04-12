#include <iostream>
#include <functional>
#include <stdio.h>
#include <ostream>

using namespace std;

auto main(int argc, char **argv) -> int {

	// std::cout << "Hello, World\n";
	// std::cout << std::plus<int>{}(1, 1.23) << '\n';
	// std::cout << std::plus<>{}(1, 1.23) << '\n';

	// ## argv[argc+n]
	// for (int i = 0; i <= argc+1; i++) {
	// 	printf("1: %s\n", argv[i]);
	// }
	// for (char **run = &argv[argc+1]; *run; run++) {
	// 	printf("2: %s\n", *run);
	// }
	
	std::cout << "argument: " << 0 << " = " << argv[0] << '\n';
	printf("1: %s\n", argv[argc+2]);
	int test = 0;
	cin >> test;
	test += 5;
	cout << test << endl;

	string accum = "";
	for (int i = 0; i < argc; i++) {
		//this
		accum = accum + argv[i] + " ";

		//or
		string tmp = argv[i];
		// accum += argv[i] + " "; // += needs valid C++ strings, not C char*
		accum += tmp + " ";
	}
	cout << accum;
}
