#pragma once

#include "webbridge/object.h"

#include <string>

class Greeter : public webbridge::object
{
public:
	property<std::string> lastGreeting;
	event<std::string> greeted;

	std::string greet(const std::string& name)
	{
		lastGreeting = "Hello, " + name + "!";
		greeted.emit(name);
		return lastGreeting();
	}
};
