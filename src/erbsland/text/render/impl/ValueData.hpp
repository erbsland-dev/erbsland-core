#pragma once

#include "ValueDataTraits.hpp"

#include "../ValueType.hpp"

namespace erbsland::text::render::impl {

/// The shared data backend for a render value.
/// @tested{RenderValueTest}
class ValueData final {
public:
    /// Create a concrete data backend value.
    template <typename Fwd>
    explicit ValueData(Fwd &&value) : _data(std::forward<Fwd>(value)) {}

public:
    /// The type of the value.
    [[nodiscard]] auto type() const noexcept -> ValueType {
        return std::visit(
            []<typename T>([[maybe_unused]] const T &value) -> ValueType {
                if constexpr (std::is_same_v<bool, T>) {
                    return ValueType::Boolean;
                } else if constexpr (std::is_same_v<String, T>) {
                    return ValueType::Text;
                } else if constexpr (std::is_same_v<int64_t, T>) {
                    return ValueType::Integer;
                } else if constexpr (std::is_same_v<double, T>) {
                    return ValueType::Float;
                } else if constexpr (std::is_same_v<ValueList, T>) {
                    return ValueType::List;
                } else if constexpr (std::is_same_v<ValueMap, T>) {
                    return ValueType::Map;
                } else if constexpr (std::is_same_v<ValueCallbackFn, T>) {
                    return ValueType::Callback;
                } else {
                    return ValueType::Null;
                }
            },
            _data);
    }
    /// Access the stored variant.
    [[nodiscard]] auto data() const noexcept -> const ValueDataVariant & { return _data; }

private:
    ValueDataVariant _data;
};

}
