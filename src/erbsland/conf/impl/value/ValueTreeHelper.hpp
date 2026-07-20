// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringEditor_fwd.hpp"
#include "../../../text/StringList.hpp"
#include "../../TestFormat.hpp"
#include "../../Value.hpp"

#include <unordered_map>
#include <utility>
#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

/// A helper class to create value trees.
class ValueTreeHelper {
    struct Frame {
        conf::ConstValuePtr frameValue;
        text::String indent;
        bool isLast;
    };

public:
    ValueTreeHelper(conf::ConstValuePtr rootValue, const TestFormat format) noexcept :
        _rootValue(std::move(rootValue)), _format(format) {}

    ~ValueTreeHelper() = default;

    // prevent copy and move.
    ValueTreeHelper(const ValueTreeHelper &) = delete;
    ValueTreeHelper(ValueTreeHelper &&) = delete;
    auto operator=(const ValueTreeHelper &) -> ValueTreeHelper & = delete;
    auto operator=(ValueTreeHelper &&) -> ValueTreeHelper & = delete;

public:
    /// Create a visual value tree.
    /// @return A list of lines for the output.
    auto createLines() -> text::StringList;

private:
    void initStack() noexcept;

    auto popFrame() noexcept -> Frame;

    [[nodiscard]] static auto computeName(const conf::ConstValuePtr &value) noexcept -> text::String;

    [[nodiscard]] auto computePosition(const conf::ConstValuePtr &value) -> text::String;

    auto appendSourceIdentifier(text::StringEditor &positionStr, const conf::ConstValuePtr &value) -> void;

    void emitLine(
        const conf::ConstValuePtr &value,
        const text::String &name,
        const text::String &pos,
        const text::String &indent,
        const bool isLast);

    void pushChildren(const conf::ConstValuePtr &value, const text::String &indent, const bool isLast);

    void appendSourceLabels();

private:
    // setup
    conf::ConstValuePtr _rootValue;
    TestFormat _format;

    // working variables
    text::StringList _lines;
    std::size_t _labelIndex = 0;
    std::unordered_map<SourceIdentifierPtr, text::String> _labelMap;
    std::vector<std::pair<text::String, SourceIdentifierPtr>> _labelList;
    std::vector<Frame> _stack;
};

}
