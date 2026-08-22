// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteCompressionAlgorithm.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"
#include "../text/String.hpp"

#include <array>
#include <limits>

namespace erbsland::mem {

using namespace text::literals;

auto ByteCompressionAlgorithm::maximumCompressedLength(const unit::ByteLength length) const -> unit::ByteLength {
    if (_value != Lz4Block || !length.isFinite()) {
        throw err::OutOfRangeError{"Compression length is unsupported."_el};
    }
    const auto value = length.toSizeTOrThrow();
    const auto extra = value / 255U + 16U;
    if (value > std::numeric_limits<std::size_t>::max() - extra) {
        throw err::OutOfRangeError{"Compressed length exceeds the supported range."_el};
    }
    return unit::ByteLength::fromSizeTOrThrow(value + extra);
}

auto ByteCompressionAlgorithm::toString() const -> text::String {
    if (_value == Lz4Block) {
        return "lz4-block"_el;
    }
    return {};
}

auto ByteCompressionAlgorithm::fromString(const text::String &text) noexcept
    -> std::optional<ByteCompressionAlgorithm> {
    if (text == "lz4-block"_el) {
        return ByteCompressionAlgorithm{Lz4Block};
    }
    return std::nullopt;
}

auto ByteCompressionAlgorithm::fromStringOrThrow(const text::String &text) -> ByteCompressionAlgorithm {
    const auto result = fromString(text);
    if (!result.has_value()) {
        throw err::ParseError{"Unsupported byte-compression algorithm."_el};
    }
    return *result;
}

auto ByteCompressionAlgorithm::all() noexcept -> std::span<const ByteCompressionAlgorithm> {
    static constexpr auto cAlgorithms = std::array{ByteCompressionAlgorithm{Lz4Block}};
    return cAlgorithms;
}

}
