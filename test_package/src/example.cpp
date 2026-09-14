#include "Greeter.h"

#include <iostream>

int main()
{
	// Deliberately not constructing webview::webview / calling
	// webbridge::register_type<Greeter>() here: that needs the WebView2
	// runtime and a message loop, which doesn't fit an unattended
	// `conan create` run. Compiling and linking this (including the
	// generated Greeter_registration.*) is enough to prove the packaged
	// headers, lib, and code generator all work together.
	Greeter g;
	std::cout << g.greet("Conan") << "\n";
	return 0;
}
