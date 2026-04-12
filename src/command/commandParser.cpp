#include "../../include/command/commandParser.hpp"

bool CommandParser::toArgv(const RESPObject& obj, std::vector<std::string>& argv, std::string& err) {
    argv.clear();
    err.clear();

    if (obj.type != RESPType::ARRAY) {
        err = "protocol error: expected array command";
        return false;
    }

    for (const auto& element : obj.elements) {
        if (element.type != RESPType::BULK_STRING && element.type != RESPType::SIMPLE_STRING) {
            err = "protocol error: command element must be string";
            argv.clear();
            return false;
        }
        argv.push_back(element.str);
    }

    if (argv.empty()) {
        err = "protocol error: empty command";
        return false;
    }

    return true;
}
