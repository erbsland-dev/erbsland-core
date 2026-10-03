// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"
#include "U8StringAppendTools.hpp"
#include "U8StringCharReadTool.hpp"
#include "U8StringDataView.hpp"
#include "U8StringSharedStorage.hpp"

#include "../U8String_fwd.hpp"
#include "../U8StringEditor_fwd.hpp"

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

/// Iteration and transformation algorithms for UTF-8 strings.
/// @tested{U8StringModifierTest StringUnchangedResultTest}
class U8StringTransformTools final {
public:
    /// Create transformation tools for `data`.
    explicit constexpr U8StringTransformTools(const U8StringDataView &data, const bool sensitive = false) noexcept :
        _data{data}, _sensitive{sensitive} {}

public:
    /// Call a function for every decoded code point.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// @overload
    auto forEach(const ProcessCharacterWithCpIndexFn &function) const -> util::LoopResult;
    /// Return mapped storage only if the transformation changes decoded text.
    [[nodiscard]] auto transformedIfChanged(TransformCharacterFn function) const
        -> std::optional<U8StringSharedStorage>;
    /// Return changed storage truncated to a maximum decoded code-point width.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto truncatedIfChanged(
        unit::CpCount maximumWidth, TruncateMode mode, const U8StringDataView &ellipsis) const
        -> std::optional<U8StringSharedStorage>;
    /// Return changed storage padded to the requested decoded code-point length.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto alignedIfChanged(unit::CpCount length, geometry::Alignment alignment, Char fill) const
        -> std::optional<U8StringSharedStorage>;

public: // escaping.
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::ByteCount;
    /// Escape this string, returning no value when its native data is unchanged.
    /// @param source The owning string with the same data view as these tools.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscapedIfChanged(
        const U8String &source, EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> std::optional<U8StringEditor>;

private:
    /// Create a data view for a UTF-8 range.
    [[nodiscard]] auto dataView(unit::ByteRange range) const -> U8StringDataView;

private:
    U8StringDataView _data;
    bool _sensitive{false};
};

}
