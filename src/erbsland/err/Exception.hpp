// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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
    /// @overload
    explicit Exception(const std::string_view reason) noexcept : Exception(text::String{reason}) {}

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

public: // conversion
    /// Convert the error with all its details into a string.
    [[nodiscard]] virtual auto toString() const noexcept -> text::StringView;

protected:
    text::StringView _reason; ///< The reason for the exception.
};

}
