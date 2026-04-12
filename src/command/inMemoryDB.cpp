#include "../../include/command/inMemoryDB.hpp"

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
