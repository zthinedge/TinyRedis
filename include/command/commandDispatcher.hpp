#pragma once

#include "inMemoryDB.hpp"
#include <string>
#include <vector>

class CommandDispatcher {
public:
    std::string dispatch(const std::vector<std::string>& argv);

private:
    InMemoryDB db_;
};
