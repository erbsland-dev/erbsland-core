// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <memory>

namespace erbsland::test {

struct MoveAwareTestCounts final {
    int copies{0};
    int moves{0};
};

class MoveAwareTestValue final {
public:
    using CountsPtr = std::shared_ptr<MoveAwareTestCounts>;

public:
    MoveAwareTestValue() = default;
    explicit MoveAwareTestValue(const int value) : _value{value}, _counts{std::make_shared<MoveAwareTestCounts>()} {}
    MoveAwareTestValue(const MoveAwareTestValue &other) : _value{other._value}, _counts{other._counts} {
        if (_counts) {
            _counts->copies += 1;
        }
    }
    MoveAwareTestValue(MoveAwareTestValue &&other) noexcept : _value{other._value}, _counts{std::move(other._counts)} {
        if (_counts) {
            _counts->moves += 1;
        }
    }
    ~MoveAwareTestValue() = default;
    auto operator=(const MoveAwareTestValue &other) -> MoveAwareTestValue & {
        if (this != &other) {
            _value = other._value;
            _counts = other._counts;
            if (_counts) {
                _counts->copies += 1;
            }
        }
        return *this;
    }
    auto operator=(MoveAwareTestValue &&other) noexcept -> MoveAwareTestValue & {
        if (this != &other) {
            _value = other._value;
            _counts = std::move(other._counts);
            if (_counts) {
                _counts->moves += 1;
            }
        }
        return *this;
    }

public:
    [[nodiscard]] auto value() const noexcept -> int { return _value; }
    [[nodiscard]] auto counts() const noexcept -> CountsPtr { return _counts; }

public: // operators
    [[nodiscard]] auto operator<=>(const MoveAwareTestValue &other) const noexcept -> std::strong_ordering {
        return _value <=> other._value;
    }
    [[nodiscard]] auto operator==(const MoveAwareTestValue &other) const noexcept -> bool {
        return _value == other._value;
    }

private:
    int _value{0};
    CountsPtr _counts{};
};

struct MoveAwareTestValueHash final {
    [[nodiscard]] auto operator()(const MoveAwareTestValue &value) const noexcept -> std::size_t {
        return std::hash<int>{}(value.value());
    }
};

}
