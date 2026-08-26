// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Value.hpp"

#include "impl/ValueData.hpp"

#include "../Literals.hpp"
#include "../StringFormat.hpp"
#include "../ToString.hpp"

#include "../../err/LogicError.hpp"
#include "../../unit/ItemCount.hpp"

#include <variant>

namespace erbsland::text::render {

using namespace text::literals;

Value::Value(const bool value) : _storage{std::make_shared<impl::ValueData>(value)} {
}
Value::Value(String text) : _storage{std::make_shared<impl::ValueData>(std::move(text))} {
}
Value::Value(int64_t value) : _storage{std::make_shared<impl::ValueData>(value)} {
}
Value::Value(double value) : _storage{std::make_shared<impl::ValueData>(value)} {
}
Value::Value(const util::List<Value> &value) : _storage{std::make_shared<impl::ValueData>(value)} {
}
Value::Value(const StringMap<Value> &value) : _storage{std::make_shared<impl::ValueData>(value)} {
}
Value::Value(ValueCallbackFn value) : _storage{std::make_shared<impl::ValueData>(std::move(value))} {
}

auto Value::type() const noexcept -> ValueType {
    if (isNull()) {
        return ValueType::Null;
    }
    return _storage->type();
}

auto Value::isNull() const noexcept -> bool {
    return _storage == nullptr;
}

auto Value::isBoolean() const noexcept -> bool {
    return type() == ValueType::Boolean;
}

auto Value::isInteger() const noexcept -> bool {
    return type() == ValueType::Integer;
}

auto Value::isFloat() const noexcept -> bool {
    return type() == ValueType::Float;
}

auto Value::isText() const noexcept -> bool {
    return type() == ValueType::Text;
}

auto Value::isList() const noexcept -> bool {
    return type() == ValueType::List;
}

auto Value::isMap() const noexcept -> bool {
    return type() == ValueType::Map;
}

auto Value::isCallback() const noexcept -> bool {
    return type() == ValueType::Callback;
}

auto Value::itemCount() const noexcept -> unit::ItemCount {
    if (isNull()) {
        return {};
    }
    if (const auto *list = std::get_if<ValueList>(&_storage->data())) {
        return list->count();
    }
    if (const auto *map = std::get_if<ValueMap>(&_storage->data())) {
        return map->count();
    }
    return {};
}

auto Value::isTruthy() const noexcept -> bool {
    if (isNull()) {
        return false;
    }
    return std::visit(
        []<typename T>(const T &value) -> bool {
            if constexpr (std::is_same_v<T, bool>) {
                return value;
            } else if constexpr (std::is_same_v<T, String>) {
                return !value.isEmpty();
            } else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, double>) {
                return value != 0;
            } else if constexpr (std::is_same_v<T, ValueList>) {
                return !value.count().isZero();
            } else if constexpr (std::is_same_v<T, ValueMap>) {
                return !value.count().isZero();
            } else {
                return true;
            }
        },
        _storage->data());
}

auto Value::asBoolean() const -> bool {
    if (!isNull()) {
        if (const auto *value = std::get_if<bool>(&_storage->data())) {
            return *value;
        }
    }
    throw err::LogicError{"The render value is not a boolean."_el};
}

auto Value::asInteger() const -> int64_t {
    if (!isNull()) {
        if (const auto *value = std::get_if<int64_t>(&_storage->data())) {
            return *value;
        }
    }
    throw err::LogicError{"The render value is not an integer."_el};
}

auto Value::asFloat() const -> double {
    if (!isNull()) {
        if (const auto *value = std::get_if<double>(&_storage->data())) {
            return *value;
        }
    }
    throw err::LogicError{"The render value is not a floating-point number."_el};
}

auto Value::asText() const -> String {
    if (!isNull()) {
        if (const auto *value = std::get_if<String>(&_storage->data())) {
            return *value;
        }
    }
    throw err::LogicError{"The render value is not text."_el};
}

auto Value::asList() const -> const ValueList & {
    if (!isNull()) {
        if (const auto *value = std::get_if<ValueList>(&_storage->data())) {
            return *value;
        }
    }
    throw err::LogicError{"The render value is not a list."_el};
}

auto Value::asMap() const -> const ValueMap & {
    if (!isNull()) {
        if (const auto *value = std::get_if<ValueMap>(&_storage->data())) {
            return *value;
        }
    }
    throw err::LogicError{"The render value is not a map."_el};
}

auto Value::get(const unit::ItemIndex index) const -> Value {
    if (isNull()) {
        return {};
    }
    if (const auto *list = std::get_if<ValueList>(&_storage->data())) {
        return list->get(index, Value{});
    }
    return {};
}

auto Value::get(const String &name) const -> Value {
    if (isNull()) {
        return {};
    }
    if (const auto *map = std::get_if<ValueMap>(&_storage->data())) {
        return map->get(name, Value{});
    }
    return {};
}

auto Value::evaluate() const -> Value {
    if (isNull()) {
        return {};
    }
    if (const auto *callback = std::get_if<ValueCallbackFn>(&_storage->data())) {
        return (*callback)();
    }
    return *this;
}

auto Value::toString() const -> String {
    using namespace text::literals;
    if (isNull()) {
        return {};
    }
    return std::visit(
        []<typename T>(const T &value) -> String {
            if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int64_t> || std::is_same_v<T, double>) {
                return text::toString(value);
            } else if constexpr (std::is_same_v<T, String>) {
                return value;
            } else if constexpr (std::is_same_v<T, ValueList>) {
                return text::StringFormat{"List(count={})"_el}.build(value.count().toSizeT());
            } else if constexpr (std::is_same_v<T, ValueMap>) {
                return text::StringFormat{"Map(count={})"_el}.build(value.count().toSizeT());
            } else {
                return {};
            }
        },
        _storage->data());
}

}
