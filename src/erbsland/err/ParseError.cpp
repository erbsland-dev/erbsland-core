// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParseError.hpp"

#include "impl/ExceptionDiagnostic.hpp"

#include "../text/StringFormat.hpp"

namespace erbsland::err {

using namespace text;
using namespace text::literals;
using namespace unit;

ParseError::ParseError(String reason, const Position &position) noexcept :
    RuntimeError{std::move(reason)}, _position{position} {
}

ParseError::ParseError(const std::string_view reason, const Position &position) noexcept :
    ParseError{String{reason}, position} {
}

auto ParseError::toString() const noexcept -> String {
    return std::visit(
        [&]<typename T>(const T &position) -> String {
            if constexpr (std::is_same_v<T, ByteIndex>) {
                static const auto cMessageFormat = StringFormat{"{} at byte index {}"_el};
                return cMessageFormat.build(
                    reason(), String::fromInteger(position.toRawValue(), IntegerFormat::hexadecimal()));
            } else if constexpr (std::is_same_v<T, CpIndex>) {
                static const auto cMessageFormat = StringFormat{"{} at code point {}"_el};
                return cMessageFormat.build(reason(), position.toRawValue());
            } else if constexpr (std::is_same_v<T, CodeLocation>) {
                static const auto cMessageFormat = StringFormat{"{} at code location {}"_el};
                return cMessageFormat.build(reason(), position.toString());
            } else {
                return reason();
            }
        },
        _position);
}

auto ParseError::diagnostic() const -> DiagnosticConstPtr {
    auto result = std::make_shared<impl::ExceptionDiagnostic>(toString());
    std::visit(
        [&]<typename T>(const T &position) -> void {
            if constexpr (std::is_same_v<T, CpIndex>) {
                auto location = CodeLocation{};
                location.setPosition(position);
                result->setLocation(location);
            } else if constexpr (std::is_same_v<T, CodeLocation>) {
                result->setLocation(position);
            } else {
                // ignore a byte index and no index
            }
        },
        _position);
    return result;
}

auto ParseError::hasPosition() const noexcept -> bool {
    return !holds_alternative<std::monostate>(_position);
}

auto ParseError::position() const noexcept -> const Position & {
    return _position;
}

}
