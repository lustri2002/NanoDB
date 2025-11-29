//
// Created by Alessio Lustri on 26/11/25.
//

#pragma once
#include <string>
#include <optional>

template <typename K, typename V>
class DBEngine {
public:
    virtual ~DBEngine() = default;

    virtual void put(const K& key, const V& value) = 0;

    virtual std::optional<V> get(const K& key) const = 0;

    virtual bool remove(const K& key) = 0;
};
