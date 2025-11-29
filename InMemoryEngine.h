//
// Created by Alessio Lustri on 26/11/25.
//

#pragma once
#include <unordered_map>
#include "DBEngine.h"

template <typename K, typename V>
class InMemoryEngine : public DBEngine<K, V> {
private:
    std::unordered_map<K, V> data;
public:
    void put(const K& key, const V& value) override {
        data[key] = value;
    }
    std::optional<V> get(const K& key) const override {
        auto iterator = data.find(key);
        if (iterator != data.end()) return iterator->second;
        else return std::nullopt;
    }
    bool remove(const K& key) override {
        return (data.erase(key) > 0) ? true : false;
    }
};