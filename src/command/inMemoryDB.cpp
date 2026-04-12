#include "../../include/command/inMemoryDB.hpp"

#include <limits>
#include <stdexcept>

void InMemoryDB::set(const std::string& key, const std::string& value) {
    kv_[key] = value;
}

bool InMemoryDB::get(const std::string& key, std::string& value) const {
    const auto it = kv_.find(key);
    if (it == kv_.end()) {
        return false;
    }
    value = it->second;
    return true;
}

int InMemoryDB::del(const std::string& key) {
    return static_cast<int>(kv_.erase(key));
}

bool InMemoryDB::exists(const std::string& key) const {
    return kv_.find(key) != kv_.end();
}

bool InMemoryDB::incr(const std::string& key, long long& newValue, std::string& err) {
    err.clear();
    long long value = 0;

    const auto it = kv_.find(key);
    if (it != kv_.end()) {
        try {
            size_t parsed = 0;
            value = std::stoll(it->second, &parsed, 10);
            if (parsed != it->second.size()) {
                err = "value is not an integer or out of range";
                return false;
            }
        } catch (const std::exception&) {
            err = "value is not an integer or out of range";
            return false;
        }
    }

    if (value == std::numeric_limits<long long>::max()) {
        err = "increment or decrement would overflow";
        return false;
    }

    newValue = value + 1;
    kv_[key] = std::to_string(newValue);
    return true;
}
