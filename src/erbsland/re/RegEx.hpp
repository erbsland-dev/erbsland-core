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

#include <atomic>
#include <functional>
#include <mutex>

namespace erbsland::re {

/// A regular expression that is compiled eagerly or on its first use.
///
/// The instance is immutable and can safely be shared between threads. A lazy expression compiles its engine exactly
/// once on first use. If compilation fails, the matching operation throws `RegExError`; a later operation retries
/// compilation.
/// All Core-string overloads decode malformed units as replacement characters. Exceptions from custom inputs propagate
/// unchanged.
/// @tested{RegExAlternativePrioritiesTest RegExCaptureGroupTest RegExLazyCompileTest RegExUtf16Utf32Test}
class RegEx final {
    friend class diagnostics::Disassembler;
    friend class diagnostics::Assembler;

    /// Mutable state retained by a lazily compiled expression.
    struct LazyState final {
        /// Create lazy compilation state.
        LazyState(Flags flags, Settings settings) noexcept;

        Flags flags;                        ///< The source pattern flags.
        Settings settings;                  ///< The source parser and engine settings.
        impl::ConstEnginePtr engine;        ///< The engine compiled on first use.
        std::once_flag compileOnce;         ///< Ensures the engine is compiled only once at a time.
        std::atomic_bool isCompiled{false}; ///< Whether compilation completed successfully.
    };

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
    [[nodiscard]] static auto compile(text::AnyString pattern, Flags flags = {}, const Settings &settings = {})
        -> RegExPtr;
    /// Create a regular expression whose engine is compiled on first use.
    /// The pattern is retained without validation. Call `compileNow()` to explicitly trigger compilation.
    /// @param pattern The regular expression pattern.
    /// @param flags The initial flags for the regular expression.
    /// @param settings The settings for the parser and resulting engine.
    /// @return A shared pointer to the lazy regular expression.
    [[nodiscard]] static auto lazyCompile(text::AnyString pattern, Flags flags = {}, const Settings &settings = {})
        -> RegExPtr;

    // defaults
    ~RegEx() = default;
    RegEx(const RegEx &) = default;
    RegEx(RegEx &&) = default;
    auto operator=(const RegEx &) -> RegEx & = default;
    auto operator=(RegEx &&) -> RegEx & = default;

public: // compilation
    /// Return the source pattern used to compile this regular expression.
    /// @return The preserved source pattern.
    [[nodiscard]] auto pattern() const noexcept -> const text::AnyString & { return _pattern; }
    /// Test if the engine was successfully compiled.
    /// Eager expressions always return `true`. This method does not trigger lazy compilation.
    [[nodiscard]] auto isCompiled() const noexcept -> bool;
    /// Compile a lazy expression now, or do nothing if its engine is already available.
    /// @throws RegExError If the pattern is invalid.
    void compileNow() const;

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
    /// Build the compiled engine for a pattern and settings.
    [[nodiscard]] static auto buildEngine(const text::AnyString &pattern, Flags flags, const Settings &settings)
        -> impl::ConstEnginePtr;
    /// Compile the retained pattern if it has not been compiled yet.
    void ensureEngineCompiled() const;
    /// Get the compiled regular-expression engine.
    [[nodiscard]] auto engine() const -> const impl::ConstEnginePtr &;

public:
    struct PrivateTag {};
    /// @internal Create an instance from a compiled engine.
    /// @param engine The pattern engine.
    /// @param pattern The source pattern.
    explicit RegEx(impl::ConstEnginePtr engine, text::AnyString pattern, PrivateTag) noexcept;
    /// @internal Create a lazy instance from a pattern and compiler options.
    /// @param pattern The source pattern.
    /// @param flags The initial pattern flags.
    /// @param settings The parser and engine settings.
    explicit RegEx(text::AnyString pattern, Flags flags, Settings settings, PrivateTag);

private:
    impl::ConstEnginePtr _engine;          ///< The eagerly compiled regular expression engine.
    text::AnyString _pattern;              ///< The source pattern.
    std::shared_ptr<LazyState> _lazyState; ///< Shared state for a lazily compiled engine.
};

}
