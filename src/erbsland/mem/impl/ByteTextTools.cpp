// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteTextTools.hpp"

#include "../ByteTextOptions.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../math/SaturatingMath.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEncoder.hpp"

namespace erbsland::mem::impl {

using namespace text::literals;

auto ByteTextTools::unitSize() const noexcept -> std::size_t {
    if (_options.encoding().isUtf8()) {
        return 1U;
    }
    return _options.encoding().isUtf16() ? 2U : 4U;
}

auto ByteTextTools::encodedEndMark() const -> ByteBlock {
    if (!_options.endMark().has_value()) {
        return {};
    }
    return text::StringEncoder{text::String::fromCharacter(*_options.endMark())}.encode(
        _options.encoding(), text::StringBomMode::Reject);
}

auto ByteTextTools::byteLengthFromCountOrThrow(const uint64_t count) const -> unit::ByteLength {
    const auto unitSizeValue = unitSize();
    const auto nativeUnitSize = static_cast<uint64_t>(unitSizeValue);
    if (math::willMultiplyOverflow(count, nativeUnitSize)) {
        throw err::OutOfRangeError("Text count exceeds the available byte range"_el);
    }
    const auto byteCount = count * nativeUnitSize;
    if (byteCount > unit::ByteLength::cRawMaximum) {
        throw err::OutOfRangeError("Text count exceeds the available byte range"_el);
    }
    return unit::ByteLength{byteCount};
}

}
