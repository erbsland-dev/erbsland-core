// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Diagnostic.hpp"

#include "../mem/UnsafeCharPtr.hpp"
#include "../text/StringView.hpp"

#include <exception>
#include <string>
#include <utility>

namespace erbsland::err {

/// The base class for all exceptions in this library.
class Exception : public std::exception {
public:
    /// Create an empty exception.
    Exception() noexcept = default;

    /// Create an error with the given reason text.
    /// @param reason The reason for the exception.
    explicit Exception(text::StringView reason) noexcept : _reason{std::move(reason)} {}
    /// Create an error with the given reason text and diagnostic cause.
    /// @param reason The reason for the exception.
    /// @param cause The diagnostic cause.
    explicit Exception(text::StringView reason, std::exception_ptr cause) noexcept :
        _reason{std::move(reason)}, _cause{std::move(cause)} {}
    /// @overload
    explicit Exception(const std::string_view reason) noexcept : Exception(text::String{reason}) {}
    /// @overload
    explicit Exception(const std::string_view reason, std::exception_ptr cause) noexcept :
        Exception{text::String{reason}, std::move(cause)} {}

    // defaults
    ~Exception() override = default;
    Exception(const Exception &) = default;
    Exception(Exception &&) = default;
    auto operator=(const Exception &) -> Exception & = default;
    auto operator=(Exception &&) -> Exception & = default;

public: // implement std::exception
    /// Return the exception reason as a null-terminated C string.
    [[nodiscard]] auto what() const noexcept -> mem::UnsafeConstCharPtr override;

public: // getters
    /// Get the reason text for the exception.
    [[nodiscard]] auto reason() const noexcept -> const text::StringView & { return _reason; }
    /// Test if this exception has a diagnostic cause.
    [[nodiscard]] auto hasCause() const noexcept -> bool { return _cause != nullptr; }
    /// Get the diagnostic cause.
    [[nodiscard]] auto cause() const noexcept -> std::exception_ptr { return _cause; }

public: // conversion
    /// Convert the error with all its details into a string.
    [[nodiscard]] virtual auto toString() const noexcept -> text::StringView;
    /// Convert the error with all its details into a structured diagnostic.
    [[nodiscard]] virtual auto diagnostic() const -> DiagnosticConstPtr;

protected:
    text::StringView _reason;  ///< The reason for the exception.
    std::exception_ptr _cause; ///< Optional diagnostic cause.
};

}
