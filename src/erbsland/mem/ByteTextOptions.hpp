// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteIntegerFormat.hpp"
#include "ByteTextFormat.hpp"
#include "ByteTextOptions_fwd.hpp"

#include "../text/Char.hpp"
#include "../text/StringEncoding.hpp"
#include "../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::mem {

/// Options that define the byte framing of a text value.
/// @tested{ByteReaderWriterTest}
class ByteTextOptions final {
public:
    /// Create default dynamic UTF-8 framing with an unsigned 32-bit count.
    constexpr ByteTextOptions() noexcept = default;
    /// Create default dynamic framing for an encoding.
    /// @param encoding The text encoding.
    constexpr ByteTextOptions(text::StringEncoding encoding) noexcept :
        _encoding{encoding} {} // NOLINT(*-explicit-constructor)
    /// Create framing with a format and encoding.
    /// @param format The byte-level framing mode.
    /// @param encoding The text encoding.
    constexpr ByteTextOptions(
        ByteTextFormat format, text::StringEncoding encoding = text::StringEncoding::Utf8) noexcept :
        _format{format}, _encoding{encoding} {}

    // defaults
    ~ByteTextOptions() = default;
    ByteTextOptions(const ByteTextOptions &) = default;
    ByteTextOptions(ByteTextOptions &&) = default;
    auto operator=(const ByteTextOptions &) -> ByteTextOptions & = default;
    auto operator=(ByteTextOptions &&) -> ByteTextOptions & = default;

public: // accessors
    /// Get the byte-level framing mode.
    [[nodiscard]] constexpr auto format() const noexcept -> ByteTextFormat { return _format; }
    /// Set the byte-level framing mode.
    constexpr auto setFormat(ByteTextFormat value) noexcept -> ByteTextOptions & {
        _format = value;
        return *this;
    }
    /// Get the text encoding.
    [[nodiscard]] constexpr auto encoding() const noexcept -> text::StringEncoding { return _encoding; }
    /// Set the text encoding.
    constexpr auto setEncoding(text::StringEncoding value) noexcept -> ByteTextOptions & {
        _encoding = value;
        return *this;
    }
    /// Get the maximum payload length or fixed field length.
    [[nodiscard]] constexpr auto length() const noexcept -> unit::ByteLength { return _length; }
    /// Set the maximum payload length or fixed field length.
    constexpr auto setLength(unit::ByteLength value) noexcept -> ByteTextOptions & {
        _length = value;
        return *this;
    }
    /// Get the optional end-mark character.
    [[nodiscard]] constexpr auto endMark() const noexcept -> const std::optional<text::Char> & { return _endMark; }
    /// Set the end-mark character.
    constexpr auto setEndMark(text::Char value) noexcept -> ByteTextOptions & {
        _endMark = value;
        return *this;
    }
    /// Clear the end-mark character.
    constexpr auto clearEndMark() noexcept -> ByteTextOptions & {
        _endMark.reset();
        return *this;
    }
    /// Get the optional code-unit count format.
    [[nodiscard]] constexpr auto countFormat() const noexcept -> const std::optional<ByteIntegerFormat> & {
        return _countFormat;
    }
    /// Set the code-unit count format.
    constexpr auto setCountFormat(ByteIntegerFormat value) noexcept -> ByteTextOptions & {
        _countFormat = value;
        return *this;
    }
    /// Clear the code-unit count format.
    constexpr auto clearCountFormat() noexcept -> ByteTextOptions & {
        _countFormat.reset();
        return *this;
    }
    /// Get the byte used to pad fixed fields.
    [[nodiscard]] constexpr auto padding() const noexcept -> Byte { return _padding; }
    /// Set the byte used to pad fixed fields.
    constexpr auto setPadding(Byte value) noexcept -> ByteTextOptions & {
        _padding = value;
        return *this;
    }

public: // factories
    /// Create the default compact dynamic framing with a variable-length count.
    [[nodiscard]] static constexpr auto compact() noexcept -> ByteTextOptions {
        auto result = ByteTextOptions{};
        result.setCountFormat(ByteIntegerFormat::UnsignedVariableLength);
        return result;
    }

private:
    ByteTextFormat _format{ByteTextFormat::Dynamic};            ///< The byte-level framing mode.
    text::StringEncoding _encoding{text::StringEncoding::Utf8}; ///< The text encoding.
    unit::ByteLength _length{unit::ByteLength::infinite()};     ///< The payload limit or fixed field length.
    std::optional<text::Char> _endMark;                         ///< The optional end-mark character.
    std::optional<ByteIntegerFormat> _countFormat{ByteIntegerFormat::UnsignedFixed32Bit}; ///< The count format.
    Byte _padding{}; ///< The fixed-field padding byte.
};

}
