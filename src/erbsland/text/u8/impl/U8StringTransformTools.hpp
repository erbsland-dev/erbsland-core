// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"
#include "U8StringAppendTools.hpp"
#include "U8StringCharReadTool.hpp"
#include "U8StringDataView.hpp"
#include "U8StringSharedStorage.hpp"

#include "../U8StringEditor_fwd.hpp"

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

/// Iteration and transformation algorithms for UTF-8 strings.
/// @tested{U8StringModifierTest}
class U8StringTransformTools final {
public:
    explicit constexpr U8StringTransformTools(const U8StringDataView &data) noexcept : _data{data} {}

public:
    /// Call a function for every decoded code point.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return storage where each decoded code point is mapped through a function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U8StringSharedStorage;
    /// Return mapped storage only if the transformation changes decoded text.
    [[nodiscard]] auto transformedIfChanged(TransformCharacterFn function) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U8StringDataView &ellipsis) const
        -> U8StringSharedStorage;
    /// Return storage padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill) const
        -> U8StringSharedStorage;

public: // escaping.
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::ByteLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> U8StringEditor;
    /// Create a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags) const -> U8StringEditor;

private:
    [[nodiscard]] auto dataView(unit::ByteRange range) const -> U8StringDataView;

private:
    U8StringDataView _data;
};

}
