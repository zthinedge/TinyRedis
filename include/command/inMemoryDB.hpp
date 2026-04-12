#pragma once

#include <string>
#include <unordered_map>

class InMemoryDB {
public:
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value) const;
    int del(const std::string& key);
    bool exists(const std::string& key) const;
    bool incr(const std::string& key, long long& newValue, std::string& err);

private:
    std::unordered_map<std::string, std::string> kv_;
};
