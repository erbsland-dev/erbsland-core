// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionAlgorithm.hpp"

#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"
#include "../text/String.hpp"

#include <array>

namespace erbsland::compression {

using namespace text::literals;

auto CompressionAlgorithm::toString() const -> text::String {
    switch (_value) {
    case Lz4Block:
        return "lz4-block"_el;
    case Deflate:
        return "deflate"_el;
    case Bzip2:
        return "bzip2"_el;
    case Lzma:
        return "lzma"_el;
    case Zstandard:
        return "zstandard"_el;
    }
    return {};
}

auto CompressionAlgorithm::fromString(const text::String &text) noexcept -> std::optional<CompressionAlgorithm> {
    if (text == "lz4-block"_el) {
        return CompressionAlgorithm{Lz4Block};
    }
    if (text == "deflate"_el) {
        return CompressionAlgorithm{Deflate};
    }
    if (text == "bzip2"_el) {
        return CompressionAlgorithm{Bzip2};
    }
    if (text == "lzma"_el) {
        return CompressionAlgorithm{Lzma};
    }
    if (text == "zstandard"_el) {
        return CompressionAlgorithm{Zstandard};
    }
    return std::nullopt;
}

auto CompressionAlgorithm::fromStringOrThrow(const text::String &text) -> CompressionAlgorithm {
    const auto result = fromString(text);
    if (!result.has_value()) {
        throw err::ParseError{"Unsupported byte-compression algorithm."_el};
    }
    return *result;
}

auto CompressionAlgorithm::all() noexcept -> std::span<const CompressionAlgorithm> {
    static constexpr auto cAlgorithms = std::array{
        CompressionAlgorithm{Lz4Block},
        CompressionAlgorithm{Deflate},
        CompressionAlgorithm{Bzip2},
        CompressionAlgorithm{Lzma},
        CompressionAlgorithm{Zstandard},
    };
    return cAlgorithms;
}

}
