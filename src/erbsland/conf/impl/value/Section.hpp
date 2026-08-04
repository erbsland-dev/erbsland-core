// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithChildren.hpp"

namespace erbsland::conf::impl {

/// A generic base class for all section-like containers.
/// This base class exists primarily to allow transforming one section type into another.
class Section : public ValueWithChildren {
public:
    /// Creates a section with the specified value type.
    /// @param valueType The initial section type.
    explicit Section(const ValueType valueType) : _valueType{valueType} {}

public:
    [[nodiscard]] auto type() const noexcept -> ValueType override { return _valueType; }

    void transform(const ValueType targetType) override {
        if (_valueType == ValueType::IntermediateSection) {
            if (targetType != ValueType::SectionWithNames && targetType != ValueType::SectionWithTexts) {
                throw err::LogicError{"Cannot convert intermediate section into the chosen type."};
            }
        } else if (_valueType == ValueType::SectionWithNames) {
            if (targetType != ValueType::SectionWithTexts) {
                throw err::LogicError{"Cannot convert section with names into the chosen type."};
            }
        } else {
            throw err::LogicError{"Cannot convert section into the chosen type."};
        }
        _valueType = targetType;
        if (targetType == ValueType::SectionWithTexts) {
            _children.setTextIndexesAllowed(true);
        }
    }

private:
    ValueType _valueType;
};

}
