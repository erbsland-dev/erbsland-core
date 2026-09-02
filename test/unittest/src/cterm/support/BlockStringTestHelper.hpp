// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestHelper.hpp"

#include <erbsland/block/StdFormat.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

/// Renders terminal block strings into standard strings for test assertions.
/// @notest{This helper is exercised by terminal test suites that inherit from it.}
class BlockStringTestHelper : public TestHelper {
public:
    /// Render an editable block string as text.
    [[nodiscard]] auto render(const BlockStringEditor &text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    /// Render an immutable block string as text.
    [[nodiscard]] auto render(const BlockString text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += blockToStdString(character);
        }
        return result;
    }

    /// Render Unicode text as terminal blocks.
    [[nodiscard]] auto render(const std::u32string &text) -> std::string {
        auto result = std::string{};
        for (const auto character : text) {
            result += blockToStdString(Block{character});
        }
        return result;
    }

    /// Render multiple block-string lines as text lines.
    [[nodiscard]] auto renderLines(const BlockStringLines &lines) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(lines.size());
        for (const auto &line : lines) {
            result.push_back(render(line));
        }
        return result;
    }

    /// Render editable block-string words as text.
    [[nodiscard]] auto renderWords(const std::vector<BlockStringEditor> &words) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(words.size());
        for (const auto &word : words) {
            result.push_back(render(word));
        }
        return result;
    }

    /// Render immutable block-string words as text.
    [[nodiscard]] auto renderWords(const std::vector<BlockString> &words) -> std::vector<std::string> {
        auto result = std::vector<std::string>{};
        result.reserve(words.size());
        for (const auto word : words) {
            result.push_back(render(word));
        }
        return result;
    }

    /// Convert an editable block string to unstyled plain text.
    [[nodiscard]] auto toPlainText(const BlockStringEditor &text) -> std::string {
        auto result = std::string{};
        for (const auto &character : text) {
            result += erbsland::text::StringConverter{character.toString()}.toStdString();
        }
        return result;
    }

    /// Require an editable block string to equal Unicode text.
    void requireStringEqual(const BlockStringEditor &actual, const std::u32string_view expected) {
        if (actual.length().toSizeT() != expected.size()) {
            throw std::runtime_error(
                std::format(
                    "BlockStringEditor length mismatch: actual={} expected={}",
                    actual.length().toSizeT(),
                    expected.size()));
        }
        for (std::size_t index = 0; index < expected.size(); ++index) {
            if (actual[BlockIndex::fromSizeT(index)] != expected[index]) {
                throw std::runtime_error(std::format("BlockStringEditor mismatch at index {}", index));
            }
        }
    }
};
