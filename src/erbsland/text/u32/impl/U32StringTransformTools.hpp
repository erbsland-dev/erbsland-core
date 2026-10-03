// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32Encoding.hpp"
#include "U32StringAppendTools.hpp"
#include "U32StringDataView.hpp"
#include "U32StringSharedStorage.hpp"

#include "../U32String_fwd.hpp"
#include "../U32StringEditor_fwd.hpp"

#include "../../../geometry/Alignment.hpp"
#include "../../../unit/CpCount.hpp"
#include "../../../util/LoopResult.hpp"
#include "../../EscapeAmount.hpp"
#include "../../EscapeFormat.hpp"
#include "../../ProcessCharacterFn.hpp"
#include "../../TransformCharacterFn.hpp"
#include "../../TruncateMode.hpp"

#include <optional>

namespace erbsland::text::impl {

/// Iteration and transformation algorithms for UTF-32 strings.
/// @tested{U32StringTest StringUnchangedResultTest}
class U32StringTransformTools final {
public:
    /// Create transformation tools for `data`.
    explicit constexpr U32StringTransformTools(const U32StringDataView &data) noexcept : _data{data} {}

public:
    /// Call a function for every decoded code point.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return mapped storage only if the transformation changes decoded text.
    [[nodiscard]] auto transformedIfChanged(TransformCharacterFn function) const
        -> std::optional<U32StringSharedStorage>;
    /// Return changed storage truncated to a maximum decoded code-point width.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto truncatedIfChanged(
        unit::CpCount maximumWidth, TruncateMode mode, const U32StringDataView &ellipsis) const
        -> std::optional<U32StringSharedStorage>;
    /// Return changed storage padded to the requested decoded code-point length.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto alignedIfChanged(unit::CpCount length, geometry::Alignment alignment, Char fill) const
        -> std::optional<U32StringSharedStorage>;

public: // escaping.
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::CpCount;
    /// Escape this string, returning no value when its native data is unchanged.
    /// @param source The owning string with the same data view as these tools.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscapedIfChanged(
        const U32String &source, EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> std::optional<U32StringEditor>;

private:
    /// Create a data view for a UTF-32 range.
    [[nodiscard]] auto dataView(unit::CpRange range) const -> U32StringDataView;

private:
    U32StringDataView _data;
};

}
