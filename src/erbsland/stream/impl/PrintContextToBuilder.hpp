// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PrintContextCommonBuilder.hpp"

#include "../../text/AnyStringBuilder.hpp"

namespace erbsland::stream::impl {

/// A print context that writes directly to a string builder.
/// @tested{AnyStringBuilderStreamTest}
class PrintContextToBuilder : public PrintContextCommonBuilder {
public:
    /// Create a new print context that writes directly to the given string builder.
    /// This print context only exists for the time of a `print` call; therefore, it is safe to use a
    /// reference to the string builder that is part of the calling instance.
    explicit PrintContextToBuilder(text::AnyStringBuilder &builder) : _builder{builder} {}

    // defaults/deletions
    ~PrintContextToBuilder() override = default;
    PrintContextToBuilder(const PrintContextToBuilder &) = delete;
    PrintContextToBuilder(PrintContextToBuilder &&) = delete;
    auto operator=(const PrintContextToBuilder &) -> PrintContextToBuilder & = delete;
    auto operator=(PrintContextToBuilder &&) -> PrintContextToBuilder & = delete;

public:
    auto commit() -> StreamWriteStatus override {
        // nothing to do, everything is already written to the string builder.
        return StreamWriteStatus::Success;
    }

protected:
    [[nodiscard]] auto builder() -> text::AnyStringBuilder & override { return _builder; }

private:
    text::AnyStringBuilder &_builder;
};

}
