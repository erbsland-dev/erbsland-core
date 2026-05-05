// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32Encoding.hpp"
#include "U32StringAppendTools.hpp"
#include "U32StringDataView.hpp"
#include "U32StringSharedStorage.hpp"

#include "../U32String_fwd.hpp"

#include "../../../bgeo/Alignment.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../util/LoopResult.hpp"
#include "../../EscapeAmount.hpp"
#include "../../EscapeFormat.hpp"
#include "../../ProcessCharacterFn.hpp"
#include "../../SafeStringFlag.hpp"
#include "../../TransformCharacterFn.hpp"
#include "../../TruncateMode.hpp"

#include <optional>

namespace erbsland::text::impl {

/// Iteration and transformation algorithms for UTF-32 strings.
/// @tested{U32StringTest}
class U32StringTransformTools final {
public:
    explicit constexpr U32StringTransformTools(const U32StringDataView &data) noexcept : _data{data} {}

public:
    /// Call a function for every decoded code point.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return storage where each decoded code point is mapped through a function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U32StringSharedStorage;
    /// Return mapped storage only if the transformation changes decoded text.
    [[nodiscard]] auto transformedIfChanged(TransformCharacterFn function) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(
        unit::CpLength maximumWidth, TruncateMode mode, const U32StringDataView &ellipsis) const
        -> U32StringSharedStorage;
    /// Return storage padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill) const
        -> U32StringSharedStorage;

public: // escaping.
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::CpLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const -> U32String;
    /// Create a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags) const -> U32String;

private:
    [[nodiscard]] auto dataView(unit::CpRange range) const -> U32StringDataView;

private:
    U32StringDataView _data;
};

}
