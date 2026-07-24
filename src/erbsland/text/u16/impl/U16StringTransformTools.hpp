// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16Encoding.hpp"
#include "U16StringAppendTools.hpp"
#include "U16StringCharReadTool.hpp"
#include "U16StringDataView.hpp"
#include "U16StringSharedStorage.hpp"

#include "../U16StringEditor_fwd.hpp"

#include "../../../bgeo/Alignment.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../../util/LoopResult.hpp"
#include "../../EscapeAmount.hpp"
#include "../../EscapeFormat.hpp"
#include "../../ProcessCharacterFn.hpp"
#include "../../SafeStringFlag.hpp"
#include "../../TransformCharacterFn.hpp"
#include "../../TruncateMode.hpp"

#include <optional>

namespace erbsland::text::impl {

/// Iteration and transformation algorithms for UTF-16 strings.
/// @tested{U16StringTest}
class U16StringTransformTools final {
public:
    explicit constexpr U16StringTransformTools(const U16StringDataView &data) noexcept : _data{data} {}

public:
    /// Call a function for every decoded code point.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return mapped storage only if the transformation changes decoded text.
    [[nodiscard]] auto transformedIfChanged(TransformCharacterFn function) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(
        unit::CpLength maximumWidth, TruncateMode mode, const U16StringDataView &ellipsis) const
        -> U16StringSharedStorage;
    /// Return storage padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill) const
        -> U16StringSharedStorage;

public: // escaping.
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::U16DataLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> U16StringEditor;
    /// Create a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags) const -> U16StringEditor;

private:
    [[nodiscard]] auto dataView(unit::U16DataRange range) const -> U16StringDataView;

private:
    U16StringDataView _data;
};

}
