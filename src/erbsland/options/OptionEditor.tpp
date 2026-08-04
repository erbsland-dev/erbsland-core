// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionError.hpp"
#include "OptionValue.hpp"

#include "../err/ParseError.hpp"

namespace erbsland::options {

template <typename tValueType>
auto OptionEditor::setValidateTextValue(text::String errorTitle) -> OptionEditor & {
    using namespace text::literals;

    _option->setValidateFn([errorTitle](const OptionValuePtr &valueToValidate, OptionValuesPtr) -> void {
        const auto valueText = valueToValidate->getText();

        if constexpr (requires { tValueType::fromStringOrThrow(valueText); }) {
            try {
                // anti-pattern: allow static_cast_void -- The conversion result is intentionally ignored after
                //     successful validation; parse errors are converted to an OptionError below.
                static_cast<void>(tValueType::fromStringOrThrow(valueText));
            } catch (const err::ParseError &parseError) {
                throw OptionError({errorTitle, parseError.reason()});
            } catch (const err::Exception &) {
                throw OptionError({errorTitle, "Failed to parse the given option text"_el});
            }
        } else if constexpr (requires {
                                 { tValueType::isValidString(valueText) } -> std::same_as<bool>;
                             }) {
            if (!tValueType::isValidString(valueText)) {
                throw OptionError({errorTitle, "The option value is invalid"_el});
            }
        } else if constexpr (requires {
                                 { tValueType::fromString(valueText) } -> std::same_as<std::optional<tValueType>>;
                             }) {
            if (!tValueType::fromString(valueText).has_value()) {
                throw OptionError({errorTitle, "The option value is invalid"_el});
            }
        } else {
            static_assert(
                requires { tValueType::fromStringOrThrow(valueText); },
                "tValueType must provide fromStringOrThrow(), isValidString(), or fromString()");
        }
    });
    return *this;
}

}
