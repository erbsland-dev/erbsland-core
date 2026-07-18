// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Flags.hpp"
#include "Input16_fwd.hpp"
#include "Input32_fwd.hpp"
#include "Input_fwd.hpp"
#include "Match16_fwd.hpp"
#include "Match32_fwd.hpp"
#include "Match_fwd.hpp"
#include "RegEx_fwd.hpp"
#include "Settings.hpp"

#include "diagnostics/Assembler_fwd.hpp"
#include "diagnostics/Disassembler_fwd.hpp"
#include "impl/engine/Engine_fwd.hpp"

#include "../stream/TextInputStream_fwd.hpp"
#include "../text/String_fwd.hpp"
#include "../text/StringCharReader_fwd.hpp"
#include "../text/u16/U16String_fwd.hpp"
#include "../text/u32/U32String_fwd.hpp"

#include <functional>

namespace erbsland::re {

/// A compiled regular expression.
///
/// The instance is immutable after compilation and can safely be shared between threads.
/// All Core-string overloads decode malformed units as replacement characters. Exceptions from custom inputs propagate
/// unchanged.
/// @tested{RegExAlternativePrioritiesTest RegExCaptureGroupTest RegExUtf16Utf32Test}
class RegEx final {
    friend class diagnostics::Disassembler;
    friend class diagnostics::Assembler;

public:
    /// A callback that creates replacement text for one match.
    using ReplaceFn = std::function<text::String(MatchPtr)>;

public:
    /// Compile a regular expression from a UTF-8 pattern.
    /// @param pattern The regular expression pattern.
    /// @param flags The initial flags for the regular expression.
    /// @param settings The settings for the parser and resulting engine.
    /// @return A shared pointer to the compiled regular expression.
    /// @throws RegExError If the pattern is invalid.
    [[nodiscard]] static auto compile(const text::String &pattern, Flags flags = {}, Settings settings = {})
        -> RegExPtr;
    /// Compile a regular expression from a UTF-16 pattern.
    /// @param pattern The regular expression pattern.
    /// @param flags The initial flags for the regular expression.
    /// @param settings The settings for the parser and resulting engine.
    /// @return A shared pointer to the compiled regular expression.
    /// @throws RegExError If the pattern is invalid.
    [[nodiscard]] static auto compile(const text::U16String &pattern, Flags flags = {}, Settings settings = {})
        -> RegExPtr;
    /// Compile a regular expression from a UTF-32 pattern.
    /// @param pattern The regular expression pattern.
    /// @param flags The initial flags for the regular expression.
    /// @param settings The settings for the parser and resulting engine.
    /// @return A shared pointer to the compiled regular expression.
    /// @throws RegExError If the pattern is invalid.
    [[nodiscard]] static auto compile(const text::U32String &pattern, Flags flags = {}, Settings settings = {})
        -> RegExPtr;

    // defaults
    ~RegEx() = default;
    RegEx(const RegEx &) = default;
    RegEx(RegEx &&) = default;
    auto operator=(const RegEx &) -> RegEx & = default;
    auto operator=(RegEx &&) -> RegEx & = default;

public: // match
    /// Try to match this expression at the start of UTF-8 text.
    [[nodiscard]] auto match(const text::String &text) const -> MatchPtr;
    /// Try to match this expression at the start of UTF-16 text.
    [[nodiscard]] auto match(const text::U16String &text) const -> Match16Ptr;
    /// Try to match this expression at the start of UTF-32 text.
    [[nodiscard]] auto match(const text::U32String &text) const -> Match32Ptr;
    /// Try to match this expression at the start of a custom UTF-8 input.
    [[nodiscard]] auto match(const InputPtr &input) const -> MatchPtr;
    /// Try to match this expression at the current position of a seekable text stream.
    /// @throws err::ParameterError if the stream is null or does not support positioning.
    /// @throws stream::StreamError if the stream times out or fails.
    [[nodiscard]] auto match(const stream::TextInputStreamPtr &input) const -> MatchPtr;
    /// Try to match this expression at the start of a custom UTF-16 input.
    [[nodiscard]] auto match(const Input16Ptr &input) const -> Match16Ptr;
    /// Try to match this expression at the start of a custom UTF-32 input.
    [[nodiscard]] auto match(const Input32Ptr &input) const -> Match32Ptr;

public: // full match
    /// Try to match this expression against all UTF-8 text.
    [[nodiscard]] auto fullMatch(const text::String &text) const -> MatchPtr;
    /// Try to match this expression against all UTF-16 text.
    [[nodiscard]] auto fullMatch(const text::U16String &text) const -> Match16Ptr;
    /// Try to match this expression against all UTF-32 text.
    [[nodiscard]] auto fullMatch(const text::U32String &text) const -> Match32Ptr;
    /// Try to match this expression against the complete custom UTF-8 input.
    [[nodiscard]] auto fullMatch(const InputPtr &input) const -> MatchPtr;
    /// Try to match this expression against all remaining text in a seekable text stream.
    /// @throws err::ParameterError if the stream is null or does not support positioning.
    /// @throws stream::StreamError if the stream times out or fails.
    [[nodiscard]] auto fullMatch(const stream::TextInputStreamPtr &input) const -> MatchPtr;
    /// Try to match this expression against the complete custom UTF-16 input.
    [[nodiscard]] auto fullMatch(const Input16Ptr &input) const -> Match16Ptr;
    /// Try to match this expression against the complete custom UTF-32 input.
    [[nodiscard]] auto fullMatch(const Input32Ptr &input) const -> Match32Ptr;

public: // find first
    /// Find the first match in UTF-8 text.
    [[nodiscard]] auto findFirst(const text::String &text) const -> MatchPtr;
    /// Find the first match in UTF-16 text.
    [[nodiscard]] auto findFirst(const text::U16String &text) const -> Match16Ptr;
    /// Find the first match in UTF-32 text.
    [[nodiscard]] auto findFirst(const text::U32String &text) const -> Match32Ptr;
    /// Find the first match in a custom UTF-8 input.
    [[nodiscard]] auto findFirst(const InputPtr &input) const -> MatchPtr;
    /// Find the first match in a seekable text stream.
    /// @throws err::ParameterError if the stream is null or does not support positioning.
    /// @throws stream::StreamError if the stream times out or fails.
    [[nodiscard]] auto findFirst(const stream::TextInputStreamPtr &input) const -> MatchPtr;
    /// Find the first match in a custom UTF-16 input.
    [[nodiscard]] auto findFirst(const Input16Ptr &input) const -> Match16Ptr;
    /// Find the first match in a custom UTF-32 input.
    [[nodiscard]] auto findFirst(const Input32Ptr &input) const -> Match32Ptr;

public: // find all
    /// Lazily find all matches in UTF-8 text.
    [[nodiscard]] auto findAll(const text::String &text) const -> MatchGenerator;
    /// Lazily find all matches in UTF-16 text.
    [[nodiscard]] auto findAll(const text::U16String &text) const -> Match16Generator;
    /// Lazily find all matches in UTF-32 text.
    [[nodiscard]] auto findAll(const text::U32String &text) const -> Match32Generator;
    /// Lazily find all matches in a custom UTF-8 input.
    [[nodiscard]] auto findAll(InputPtr input) const -> MatchGenerator;
    /// Lazily find all matches in a seekable text stream.
    /// @throws err::ParameterError if the stream is null or does not support positioning.
    /// @throws stream::StreamError if the stream times out or fails.
    [[nodiscard]] auto findAll(stream::TextInputStreamPtr input) const -> MatchGenerator;
    /// Lazily find all matches in a custom UTF-16 input.
    [[nodiscard]] auto findAll(Input16Ptr input) const -> Match16Generator;
    /// Lazily find all matches in a custom UTF-32 input.
    [[nodiscard]] auto findAll(Input32Ptr input) const -> Match32Generator;

public: // collect all
    /// Collect all matches in UTF-8 text.
    [[nodiscard]] auto collectAll(const text::String &text) const -> MatchList;
    /// Collect all matches in UTF-16 text.
    [[nodiscard]] auto collectAll(const text::U16String &text) const -> Match16List;
    /// Collect all matches in UTF-32 text.
    [[nodiscard]] auto collectAll(const text::U32String &text) const -> Match32List;
    /// Collect all matches in a custom UTF-8 input.
    [[nodiscard]] auto collectAll(const InputPtr &input) const -> MatchList;
    /// Collect all matches in a seekable text stream.
    /// @throws err::ParameterError if the stream is null or does not support positioning.
    /// @throws stream::StreamError if the stream times out or fails.
    [[nodiscard]] auto collectAll(const stream::TextInputStreamPtr &input) const -> MatchList;
    /// Collect all matches in a custom UTF-16 input.
    [[nodiscard]] auto collectAll(const Input16Ptr &input) const -> Match16List;
    /// Collect all matches in a custom UTF-32 input.
    [[nodiscard]] auto collectAll(const Input32Ptr &input) const -> Match32List;

public: // replacement
    /// Replace all matches in UTF-8 text using a replacement expression.
    [[nodiscard]] auto replaceAll(const text::String &text, const text::String &replacementExpression) const
        -> text::String;
    /// Replace all matches in UTF-8 text using a callback.
    [[nodiscard]] auto replaceAll(const text::String &text, const ReplaceFn &replaceFn) const -> text::String;

private: // internal API
    struct PrivateTag {};
    [[nodiscard]] static auto compileReader(text::StringCharReader reader, Flags flags, Settings settings) -> RegExPtr;
    [[nodiscard]] auto engine() const noexcept -> const impl::ConstEnginePtr &;

public:
    /// @internal Create an instance from a compiled engine.
    explicit RegEx(impl::ConstEnginePtr engine, PrivateTag) noexcept;

private:
    impl::ConstEnginePtr _engine; ///< The compiled regular expression engine.
};

}
