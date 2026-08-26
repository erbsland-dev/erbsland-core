// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Decoder_fwd.hpp"
#include "DecoderState.hpp"
#include "Transaction_fwd.hpp"

#include "../char/CharClass.hpp"
#include "../char/NamedChars.hpp"

#include "../../../text/AsciiCategory.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../ConfError.hpp"
#include "../../Location.hpp"

#include <functional>
#include <optional>
#include <type_traits>

namespace erbsland::conf::impl {

using namespace text::literals;

/// The base class for all character-based decoders.
class Decoder {
public:
    // defaults
    Decoder() = default;
    virtual ~Decoder() = default;

    // defaults/deletions
    Decoder(const Decoder &) = delete;
    auto operator=(const Decoder &) -> Decoder & = delete;
    Decoder(Decoder &&) = delete;
    auto operator=(Decoder &&) -> Decoder & = delete;

public:
    /// Initialize this decoder.
    virtual void initialize() = 0;

    /// Access the current character.
    [[nodiscard]] virtual auto character() const noexcept -> text::Char = 0;

    /// Get the current location.
    [[nodiscard]] virtual auto location() const -> Location = 0;

    /// Access the source identifier.
    [[nodiscard]] virtual auto sourceIdentifier() const noexcept -> SourceIdentifierPtr = 0;

    /// Get a best-effort source excerpt for an error location.
    [[nodiscard]] virtual auto codeSnippet(unit::CodeLocation) const noexcept -> std::optional<text::CodeSnippet> {
        return std::nullopt;
    }

    /// Capture the current character and decode the next.
    /// @throws ConfError (Encoding) In case of any encoding error.
    virtual void next() = 0;

    /// Advance while the current character belongs to an ASCII category.
    /// Delayed decoder errors at the boundary remain pending so callers can preserve token-yield timing.
    /// @param category The ASCII category to consume.
    /// @param maximum The maximum number of characters to consume.
    /// @return The number of consumed characters.
    auto advanceWhile(text::AsciiCategory category, unit::CpLength maximum = unit::CpLength::infinite())
        -> unit::CpLength;

protected:
    /// Test if speculative parsing is currently active.
    [[nodiscard]] auto hasActiveTransaction() const noexcept -> bool { return _activeTransaction.has_value(); }

private:
    friend class Transaction;

    /// Start a nested decoder transaction and return its checkpoint.
    [[nodiscard]] auto startTransaction(Transaction &transaction) noexcept -> DecoderState;
    /// Commit the active transaction.
    /// Removing the intrusive top link is sufficient: an outer transaction keeps its original checkpoint and can
    /// still capture or restore the complete range without transferring buffered characters.
    void commitTransaction(Transaction &transaction) noexcept;
    /// Restore and roll back the active transaction.
    /// The checkpoint directly restores the character and line cursor, replacing the former character replay stack.
    void rollbackTransaction(Transaction &transaction) noexcept;
    /// Get the captured code-point count for an open transaction.
    [[nodiscard]] auto transactionCapturedSize(const Transaction &transaction) const noexcept -> std::size_t;
    /// Capture the raw text for an open transaction.
    [[nodiscard]] auto captureTransactionContent(const Transaction &transaction) const noexcept -> text::String;

    /// Capture the complete current decoder state.
    [[nodiscard]] virtual auto decoderState() const noexcept -> DecoderState = 0;
    /// Restore a previously captured decoder state.
    virtual void restoreDecoderState(const DecoderState &state) noexcept = 0;
    /// Capture a COW byte slice from a checkpoint up to, but excluding, the current character.
    [[nodiscard]] virtual auto captureFromDecoderState(const DecoderState &state) const noexcept -> text::String = 0;

public: // Throwing common exceptions.
    /// Throw the given error.
    [[noreturn]] void throwError(const ConfErrorCategory category, text::String message) const {
        checkForErrorAndThrowIt();
        const auto errorLocation = location();
        throw ConfError(category, std::move(message), errorLocation)
            .withCodeSnippet(codeSnippet(errorLocation.codeLocation()));
    }

    /// In higher layers, control-character and encoding errors need to be delayed for correct error handling.
    /// This placeholder method allows checking if an error was encountered and needs to be propagated to
    /// the calling code.
    virtual void checkForErrorAndThrowIt() const {
        // not implemented in the base decoder.
    }

    /// Throw a syntax error.
    [[noreturn]] void throwSyntaxError(text::String message) const {
        throwError(ConfErrorCategory::Syntax, std::move(message));
    }

    /// Throw a limit exceeded error.
    [[noreturn]] void throwLimitExceededError(text::String message) const {
        throwError(ConfErrorCategory::LimitExceeded, std::move(message));
    }

    /// Throw an error if a number exceeds the 64-bit limit.
    [[noreturn]] void throwNumberLimitExceededError() const {
        throwLimitExceededError("The number exceeds the 64-bit limit."_el);
    }

    /// Throw an error if the document ends at an unexpected location.
    [[noreturn]] void throwUnexpectedEndOfDataError() const {
        throwError(ConfErrorCategory::UnexpectedEnd, "Unexpected end of data."_el);
    }

    /// Throw an error if the document ends at an unexpected location.
    [[noreturn]] void throwUnexpectedEndOfDataError(text::String message) const {
        throwError(ConfErrorCategory::UnexpectedEnd, std::move(message));
    }

    /// Throws an unexpected end or syntax error, depending on the current character.
    [[noreturn]] void throwSyntaxOrUnexpectedEndError(text::String message) const {
        if (character().isEndOfData()) {
            throwUnexpectedEndOfDataError(std::move(message));
        }
        throwSyntaxError(std::move(message));
    }

    /// Throw an internal error.
    [[noreturn]] void throwInternalError(text::String message) const {
        throwError(ConfErrorCategory::Internal, std::move(message));
    }

public: // Constraining functions.
    /// Expect the given Unicode character or character class.
    template <typename T>
        requires(
            std::is_same_v<std::remove_cvref_t<T>, text::Char> || std::is_same_v<std::remove_cvref_t<T>, CharClass>)
    void expect(T expected, text::String message) {
        if (character() != expected) {
            if (character().isEndOfData()) {
                throwUnexpectedEndOfDataError(std::move(message));
            }
            throwSyntaxError(std::move(message));
        }
    }

    /// Expect and skip the given character or character class.
    template <typename T>
        requires(
            std::is_same_v<std::remove_cvref_t<T>, text::Char> || std::is_same_v<std::remove_cvref_t<T>, CharClass>)
    void expectAndNext(T expected, text::String message) {
        expect(expected, std::move(message));
        next();
    }

    /// Expect that the document continues.
    /// @param message The error message in case the character stream ends here.
    void expectMore(text::String message) const {
        if (character().isEndOfData()) {
            throwUnexpectedEndOfDataError(std::move(message));
        }
    }

private:
    /// The borrowed innermost transaction; transactions form an intrusive stack of scoped references.
    std::optional<std::reference_wrapper<Transaction>> _activeTransaction;
};

}
