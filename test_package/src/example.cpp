#include "Greeter.h"

#include <iostream>

int main()
{
	// Deliberately not constructing webview::webview / calling
	// webbridge::register_type<Greeter>() here: that needs the WebView2
	// runtime and a message loop, which doesn't fit an unattended
	// `conan create` run. There's also no generated Greeter_registration.*
	// to call it with - see the comment in ../CMakeLists.txt for why
	// webbridge_generate() isn't invoked here. Compiling, linking, and
	// running this plain C++ property/event usage is enough to prove the
	// packaged headers and lib work correctly.
	Greeter g;
	std::cout << g.greet("Conan") << "\n";
	return 0;
}
