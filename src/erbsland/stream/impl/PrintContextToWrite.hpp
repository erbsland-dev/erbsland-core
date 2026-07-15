// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PrintContextCommonBuilder.hpp"

#include "../TextOutputStream_fwd.hpp"

#include "../../text/StringBuilder.hpp"

namespace erbsland::stream::impl {

/// A print context that uses the `write` method of a text stream to print a collected line.
/// @tested{StandardTextOutputStreamTest}
class PrintContextToWrite : public PrintContextCommonBuilder {
public:
    /// Create a new print context that writes to the given output stream.
    /// This print context only exists for the time of a `print` call; therefore, it is safe to use a
    /// reference to the output stream.
    explicit PrintContextToWrite(TextOutputStream &output) : _output{output} {}

    // defaults / prevent copy and move
    ~PrintContextToWrite() override = default;
    PrintContextToWrite(const PrintContextToWrite &) = delete;
    PrintContextToWrite(PrintContextToWrite &&) = delete;
    auto operator=(const PrintContextToWrite &) -> PrintContextToWrite & = delete;
    auto operator=(PrintContextToWrite &&) -> PrintContextToWrite & = delete;

public:
    auto commit() -> StreamWriteStatus override;

protected:
    [[nodiscard]] auto builder() -> text::StringBuilder & override { return _builder; }

private:
    text::StringBuilder _builder;
    TextOutputStream &_output;
};

}
