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
    /// Stores one pending value-tree node and its rendering context.
    struct Frame {
        conf::ConstValuePtr frameValue;
        text::String indent;
        bool isLast;
    };

public:
    /// Create a helper for rendering a configuration value tree.
    /// @param rootValue The root value to render.
    /// @param format The requested output format.
    ValueTreeHelper(conf::ConstValuePtr rootValue, const TestFormat format) noexcept :
        _rootValue(std::move(rootValue)), _format(format) {}

    // defaults/deletions
    ~ValueTreeHelper() = default;
    ValueTreeHelper(const ValueTreeHelper &) = delete;
    ValueTreeHelper(ValueTreeHelper &&) = delete;
    auto operator=(const ValueTreeHelper &) -> ValueTreeHelper & = delete;
    auto operator=(ValueTreeHelper &&) -> ValueTreeHelper & = delete;

public:
    /// Create a visual value tree.
    /// @return A list of lines for the output.
    auto createLines() -> text::StringList;

private:
    /// Initialize the traversal stack with the root value.
    void initStack() noexcept;

    /// Remove and return the next pending traversal frame.
    auto popFrame() noexcept -> Frame;

    /// Compute the display name for a value.
    [[nodiscard]] static auto computeName(const conf::ConstValuePtr &value) noexcept -> text::String;

    /// Compute the source position text for a value.
    [[nodiscard]] auto computePosition(const conf::ConstValuePtr &value) -> text::String;

    /// Append a value's source identifier to position text.
    auto appendSourceIdentifier(text::StringEditor &positionStr, const conf::ConstValuePtr &value) -> void;

    /// Render one value-tree line.
    void emitLine(
        const conf::ConstValuePtr &value,
        const text::String &name,
        const text::String &pos,
        const text::String &indent,
        const bool isLast);

    /// Push a value's child nodes onto the traversal stack.
    void pushChildren(const conf::ConstValuePtr &value, const text::String &indent, const bool isLast);

    /// Append source labels collected while rendering the tree.
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
