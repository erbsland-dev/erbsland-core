// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionErrorReason.hpp"
#include "OptionSet_fwd.hpp"

#include "../text/StringView.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <utility>

namespace erbsland::options {

/// A detailed error context to pin the source and reason for an error.
/// @tested{OptionsFrameworkTest}
class OptionErrorContext {
public:
    // defaults
    OptionErrorContext() = default;
    ~OptionErrorContext() = default;
    OptionErrorContext(const OptionErrorContext &) = default;
    auto operator=(const OptionErrorContext &) -> OptionErrorContext & = default;
    OptionErrorContext(OptionErrorContext &&) noexcept = default;
    auto operator=(OptionErrorContext &&) noexcept -> OptionErrorContext & = default;

public:
    /// Get the user-facing error description.
    [[nodiscard]] auto description() const -> text::StringView { return _description; }
    /// Set the user-facing error description.
    /// @param description The description text.
    /// @return A reference to this context.
    auto setDescription(text::StringView description) -> OptionErrorContext & {
        _description = std::move(description);
        return *this;
    }
    /// Get the machine-readable error reason.
    [[nodiscard]] auto reason() const -> OptionErrorReason { return _reason; }
    /// Set the machine-readable error reason.
    /// @param reason The reason value.
    /// @return A reference to this context.
    auto setReason(OptionErrorReason reason) -> OptionErrorContext & {
        _reason = reason;
        return *this;
    }
    /// Get the command-line argument index related to the error.
    [[nodiscard]] auto argumentIndex() const -> unit::ArgumentIndex { return _argumentIndex; }
    /// Set the command-line argument index related to the error.
    /// @param index The argument index.
    /// @return A reference to this context.
    auto setArgumentIndex(unit::ArgumentIndex index) -> OptionErrorContext & {
        _argumentIndex = index;
        return *this;
    }
    /// Get the option related to the error if it still exists.
    [[nodiscard]] auto option() const -> OptionPtr { return _option.lock(); }
    /// Set the option related to the error.
    /// @param option The option pointer.
    /// @return A reference to this context.
    auto setOption(const OptionPtr &option) -> OptionErrorContext & {
        _option = option;
        return *this;
    }
    /// Get the option set related to the error if it still exists.
    [[nodiscard]] auto optionSet() const -> OptionSetPtr { return _optionSet.lock(); }
    /// Set the option set related to the error.
    /// @param optionSet The option-set pointer.
    /// @return A reference to this context.
    auto setOptionSet(const OptionSetPtr &optionSet) -> OptionErrorContext & {
        _optionSet = optionSet;
        return *this;
    }
    /// Get the module name related to the error.
    [[nodiscard]] auto moduleName() const -> text::StringView { return _moduleName; }
    /// Set the module name related to the error.
    /// @param moduleName The module name.
    /// @return A reference to this context.
    auto setModuleName(text::StringView moduleName) -> OptionErrorContext & {
        _moduleName = std::move(moduleName);
        return *this;
    }

private:
    text::StringView _description;                      ///< A textual description of the error.
    OptionErrorReason _reason{OptionErrorReason::None}; ///< Machine readable reason for the error.
    unit::ArgumentIndex _argumentIndex{
        unit::ArgumentIndex::noIndex()};                ///< The argument index where the error occurred.
    OptionWeakPtr _option;                              ///< The option where the error occurred.
    OptionSetWeakPtr _optionSet;                        ///< The option set where the error occurred.
    text::StringView _moduleName;                       ///< The module name or empty.
};

}
