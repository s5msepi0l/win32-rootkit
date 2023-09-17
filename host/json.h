#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <variant>

using JSONValue = std::variant<std::string, double, bool, std::nullptr_t, std::vector<JSONValue>, std::map<std::string, JSONValue>>;

class JSON {
private:

public:
	JSONValue parse

private:
};