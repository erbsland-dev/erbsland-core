// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "ValidationContext.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/StringList.hpp"
#include "../../ConfError.hpp"

#include <cmath>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

template <typename T>
class InConstraint : public Constraint {
public:
    using Values = std::conditional_t<std::is_same_v<T, text::String>, text::StringList, std::vector<T>>;

    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, Values>)
    explicit InConstraint(Fwd &&values) : _values(std::forward<Fwd>(values)) {
        setType(vr::ConstraintType::In);
    }

public:
    [[nodiscard]] static auto hasDuplicate(const Values &values, const text::CaseSensitivity cs) -> bool {
        for (auto first = values.begin(); first != values.end(); ++first) {
            for (auto other = std::next(first); other != values.end(); ++other) {
                if (areEqual(*first, *other, cs)) {
                    return true;
                }
            }
        }
        return false;
    }

protected:
    [[nodiscard]] auto isEqual(const T &a, const T &b, const ValidationContext &context) const -> bool {
        return areEqual(a, b, context.rule->caseSensitivity());
    }

    [[nodiscard]] static auto areEqual(const T &a, const T &b, const text::CaseSensitivity cs) -> bool {
        if constexpr (std::is_same_v<T, text::String>) {
            return a.compare(b, cs.asciiComparisonFn()) == std::strong_ordering::equal;
        } else if constexpr (std::is_same_v<T, mem::ByteBlock>) {
            return a == b;
        } else if constexpr (std::is_floating_point_v<T>) {
            return std::abs(a - b) < std::numeric_limits<T>::epsilon();
        } else {
            return a == b;
        }
    }

    [[nodiscard]] auto contains(const T &value, const ValidationContext &context) const -> bool {
        for (const auto &v : _values) {
            if (isEqual(v, value, context)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] auto isNotValid(const T &validatedValue, const ValidationContext &context) const -> bool {
        if (isNegated()) {
            // invalid if it is in the list when negated
            return contains(validatedValue, context);
        }
        // invalid if it is not in the list when not negated
        return !contains(validatedValue, context);
    }

    [[nodiscard]] auto comparisonText() const -> const text::String & {
        static const text::String inText = "must be one of"_el;
        static const text::String notInText = "must not be one of"_el;
        return isNegated() ? notInText : inText;
    }

protected:
    Values _values;
};

class InIntegerConstraint final : public InConstraint<Integer> {
public:
    explicit InIntegerConstraint(const std::vector<Integer> &values) : InConstraint(values) {}

protected:
    void validateInteger(const ValidationContext &context, Integer value) const override;
};

class InFloatConstraint final : public InConstraint<Float> {
public:
    explicit InFloatConstraint(const std::vector<Float> &values) : InConstraint(values) {}

protected:
    void validateFloat(const ValidationContext &context, Float value) const override;
};

class InTextConstraint final : public InConstraint<text::String> {
public:
    explicit InTextConstraint(const text::StringList &values) : InConstraint(values) {}

protected:
    void validateText(const ValidationContext &context, const text::String &value) const override;
};

class InBytesConstraint final : public InConstraint<mem::ByteBlock> {
public:
    explicit InBytesConstraint(const std::vector<mem::ByteBlock> &values) : InConstraint(values) {}

protected:
    void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const override;
};

auto handleInConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
