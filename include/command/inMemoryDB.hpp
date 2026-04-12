#pragma once

#include <string>
#include <unordered_map>

class InMemoryDB {
public:
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value) const;
    int del(const std::string& key);

private:
    std::unordered_map<std::string, std::string> kv_;
};
