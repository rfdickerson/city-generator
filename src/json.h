#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool b = false;
    double num = 0.0;
    std::string str;
    std::vector<JsonValue> arr;
    std::unordered_map<std::string, JsonValue> obj;
};

bool ParseJson(const std::string& text, JsonValue* out, std::string* err);
