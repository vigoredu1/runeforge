#pragma once

#include <variant>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

// forward declared — defined in interpreter.h
struct RuneCallable;

using Value = std::variant<
    std::monostate,                    // null
    double,                            // number
    bool,                              // blessed / cursed
    std::string,                       // string
    std::shared_ptr<RuneCallable>      // spell / ritual
>;

std::string valueToString(const Value& v);
bool        isTruthy(const Value& v);

class Environment {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);

    void  define(const std::string& name, const Value& val);
    Value get(const std::string& name) const;
    void  assign(const std::string& name, const Value& val);

    std::shared_ptr<Environment> parent;

private:
    std::unordered_map<std::string, Value> values;
};
