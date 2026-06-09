// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../unit/Version.hpp"

#include <utility>

namespace erbsland::core {

/// Optional application metadata used by command line rendering.
/// @tested{OptionsFrameworkTest, StandardOptionRendererTest}
class ApplicationInfo {
public:
    ApplicationInfo() = default;
    ~ApplicationInfo() = default;
    ApplicationInfo(const ApplicationInfo &) = default;
    auto operator=(const ApplicationInfo &) -> ApplicationInfo & = default;
    ApplicationInfo(ApplicationInfo &&) = default;
    auto operator=(ApplicationInfo &&) -> ApplicationInfo & = default;

public: // accessors
    /// Get the application name.
    [[nodiscard]] auto applicationName() const noexcept -> const text::StringView & { return _applicationName; }
    /// Set the application name.
    void setApplicationName(text::StringView applicationName) { _applicationName = std::move(applicationName); }
    /// Get the application version.
    [[nodiscard]] auto applicationVersion() const noexcept -> const unit::Version & { return _applicationVersion; }
    /// Set the application version.
    void setApplicationVersion(unit::Version applicationVersion) noexcept { _applicationVersion = applicationVersion; }
    /// Get the author or organization name.
    [[nodiscard]] auto authorName() const noexcept -> const text::StringView & { return _authorName; }
    /// Set the author or organization name.
    void setAuthorName(text::StringView authorName) { _authorName = std::move(authorName); }
    /// Get the copyright line.
    [[nodiscard]] auto copyrightLine() const noexcept -> const text::StringView & { return _copyrightLine; }
    /// Set the copyright line.
    void setCopyrightLine(text::StringView copyrightLine) { _copyrightLine = std::move(copyrightLine); }
    /// Get the license text.
    [[nodiscard]] auto licenseText() const noexcept -> const text::StringView & { return _licenseText; }
    /// Set the license text.
    void setLicenseText(text::StringView licenseText) { _licenseText = std::move(licenseText); }

private:
    text::StringView _applicationName; ///< The name of the application.
    unit::Version _applicationVersion; ///< The application version.
    text::StringView _authorName;      ///< The author or organization name.
    text::StringView _copyrightLine;   ///< A copyright line.
    text::StringView _licenseText;     ///< A license text.
};

}
