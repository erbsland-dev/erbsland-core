// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ToString.hpp"

#include "Literals.hpp"
#include "String.hpp"

namespace erbsland::text {

auto toString(const String &value) -> String {
    return value;
}

auto toString(const bool value, const BooleanFormat format) -> String {
    return String::fromBoolean(value, format);
}

auto toString(const std::strong_ordering value) -> String {
    using namespace literals;

    if (value == std::strong_ordering::less) {
        return "less"_el;
    }
    if (value == std::strong_ordering::greater) {
        return "greater"_el;
    }
    return "equal"_el;
}

auto toString(int8_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(int16_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(int32_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(int64_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(uint8_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(uint16_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(uint32_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(uint64_t value, IntegerFormat format) -> String {
    return String::fromInteger(value, format);
}

auto toString(float value, FloatFormat format) -> String {
    return String::fromFloat(value, format);
}

auto toString(double value, FloatFormat format) -> String {
    return String::fromFloat(value, format);
}

}
