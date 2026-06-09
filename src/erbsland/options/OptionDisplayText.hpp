// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/StringView.hpp"

#include <utility>

namespace erbsland::options {

/// Text fragments used by option renderers.
/// @tested{StandardOptionRendererTest TerminalOptionsRendererTest}
class OptionDisplayText final {
public:
    OptionDisplayText();

    // defaults
    ~OptionDisplayText() = default;
    OptionDisplayText(const OptionDisplayText &) = default;
    auto operator=(const OptionDisplayText &) -> OptionDisplayText & = default;
    OptionDisplayText(OptionDisplayText &&) = default;
    auto operator=(OptionDisplayText &&) -> OptionDisplayText & = default;

public: // accessors
    [[nodiscard]] auto applicationNameFallback() const noexcept -> const text::StringView & {
        return _applicationNameFallback;
    }
    void setApplicationNameFallback(text::StringView value) { _applicationNameFallback = std::move(value); }
    [[nodiscard]] auto usageLabel() const noexcept -> const text::StringView & { return _usageLabel; }
    void setUsageLabel(text::StringView value) { _usageLabel = std::move(value); }
    [[nodiscard]] auto modulePlaceholder() const noexcept -> const text::StringView & { return _modulePlaceholder; }
    void setModulePlaceholder(text::StringView value) { _modulePlaceholder = std::move(value); }
    [[nodiscard]] auto optionsPlaceholder() const noexcept -> const text::StringView & { return _optionsPlaceholder; }
    void setOptionsPlaceholder(text::StringView value) { _optionsPlaceholder = std::move(value); }
    [[nodiscard]] auto modulesHeading() const noexcept -> const text::StringView & { return _modulesHeading; }
    void setModulesHeading(text::StringView value) { _modulesHeading = std::move(value); }
    [[nodiscard]] auto optionsHeading() const noexcept -> const text::StringView & { return _optionsHeading; }
    void setOptionsHeading(text::StringView value) { _optionsHeading = std::move(value); }
    [[nodiscard]] auto helpOptionDescription() const noexcept -> const text::StringView & {
        return _helpOptionDescription;
    }
    void setHelpOptionDescription(text::StringView value) { _helpOptionDescription = std::move(value); }
    [[nodiscard]] auto versionOptionDescription() const noexcept -> const text::StringView & {
        return _versionOptionDescription;
    }
    void setVersionOptionDescription(text::StringView value) { _versionOptionDescription = std::move(value); }
    [[nodiscard]] auto integerPlaceholder() const noexcept -> const text::StringView & { return _integerPlaceholder; }
    void setIntegerPlaceholder(text::StringView value) { _integerPlaceholder = std::move(value); }
    [[nodiscard]] auto valuePlaceholder() const noexcept -> const text::StringView & { return _valuePlaceholder; }
    void setValuePlaceholder(text::StringView value) { _valuePlaceholder = std::move(value); }
    [[nodiscard]] auto choicePlaceholder() const noexcept -> const text::StringView & { return _choicePlaceholder; }
    void setChoicePlaceholder(text::StringView value) { _choicePlaceholder = std::move(value); }
    [[nodiscard]] auto placeholderPrefix() const noexcept -> const text::StringView & { return _placeholderPrefix; }
    void setPlaceholderPrefix(text::StringView value) { _placeholderPrefix = std::move(value); }
    [[nodiscard]] auto placeholderSuffix() const noexcept -> const text::StringView & { return _placeholderSuffix; }
    void setPlaceholderSuffix(text::StringView value) { _placeholderSuffix = std::move(value); }
    [[nodiscard]] auto choicesLabel() const noexcept -> const text::StringView & { return _choicesLabel; }
    void setChoicesLabel(text::StringView value) { _choicesLabel = std::move(value); }
    [[nodiscard]] auto versionLabel() const noexcept -> const text::StringView & { return _versionLabel; }
    void setVersionLabel(text::StringView value) { _versionLabel = std::move(value); }
    [[nodiscard]] auto authorLabel() const noexcept -> const text::StringView & { return _authorLabel; }
    void setAuthorLabel(text::StringView value) { _authorLabel = std::move(value); }
    [[nodiscard]] auto licenseLabel() const noexcept -> const text::StringView & { return _licenseLabel; }
    void setLicenseLabel(text::StringView value) { _licenseLabel = std::move(value); }
    [[nodiscard]] auto errorLabel() const noexcept -> const text::StringView & { return _errorLabel; }
    void setErrorLabel(text::StringView value) { _errorLabel = std::move(value); }
    [[nodiscard]] auto genericErrorMessage() const noexcept -> const text::StringView & { return _genericErrorMessage; }
    void setGenericErrorMessage(text::StringView value) { _genericErrorMessage = std::move(value); }
    [[nodiscard]] auto moduleLabel() const noexcept -> const text::StringView & { return _moduleLabel; }
    void setModuleLabel(text::StringView value) { _moduleLabel = std::move(value); }
    [[nodiscard]] auto optionLabel() const noexcept -> const text::StringView & { return _optionLabel; }
    void setOptionLabel(text::StringView value) { _optionLabel = std::move(value); }
    [[nodiscard]] auto argumentIndexLabel() const noexcept -> const text::StringView & { return _argumentIndexLabel; }
    void setArgumentIndexLabel(text::StringView value) { _argumentIndexLabel = std::move(value); }

public:
    /// Get the shared default display text.
    [[nodiscard]] static auto defaultText() noexcept -> const OptionDisplayText &;

private:
    text::StringView _applicationNameFallback; ///< Fallback application name.
    text::StringView _usageLabel;              ///< Usage label.
    text::StringView _modulePlaceholder;       ///< Module placeholder.
    text::StringView _optionsPlaceholder;      ///< Options placeholder.
    text::StringView _modulesHeading;          ///< Modules heading.
    text::StringView _optionsHeading;          ///< Options heading.
    text::StringView _helpOptionDescription;   ///< Help option description.
    /// Version option description.
    text::StringView _versionOptionDescription;
    text::StringView _integerPlaceholder;  ///< Integer value placeholder.
    text::StringView _valuePlaceholder;    ///< Text value placeholder.
    text::StringView _choicePlaceholder;   ///< Choice value placeholder.
    text::StringView _placeholderPrefix;   ///< Dynamic placeholder prefix.
    text::StringView _placeholderSuffix;   ///< Dynamic placeholder suffix.
    text::StringView _choicesLabel;        ///< Choices label.
    text::StringView _versionLabel;        ///< Version label.
    text::StringView _authorLabel;         ///< Author label.
    text::StringView _licenseLabel;        ///< License label.
    text::StringView _errorLabel;          ///< Error label.
    text::StringView _genericErrorMessage; ///< Generic error message.
    text::StringView _moduleLabel;         ///< Module label.
    text::StringView _optionLabel;         ///< Option label.
    text::StringView _argumentIndexLabel;  ///< Argument index label.
};

}
