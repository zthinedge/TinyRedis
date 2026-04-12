#pragma once

#include "../protocol/respObject.hpp"
#include <string>
#include <vector>

class CommandParser {
public:
    static bool toArgv(const RESPObject& obj, std::vector<std::string>& argv, std::string& err);
};
