// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TlsConfigurationParser.hpp>
#include <erbsland/TlsConfigurationSet.hpp>

namespace demo {

using namespace el::text::literals;

/// Load a configuration beside the executable and apply explicitly supplied CLI values.
class MailConfiguration {
public:
    /// Parse the selected file, preserving its location for certificate and log paths.
    /// @param values Parsed command-line values.
    /// @param defaultFile Default basename beside the executable.
    MailConfiguration(el::OptionValuesPtr values, const el::String &defaultFile) : _values{std::move(values)} {
        _path = el::Path::executablePathOrThrow().parent() / el::Path{defaultFile};
        if (_values->value("config"_el)) {
            _path = el::Path{_values->getText("config"_el)}.toAbsoluteOrThrow();
        }
        _document = el::conf::Parser{}.parseFileOrThrow(_path);
    }

public: // configuration
    /// Access the main section for schema validation and typed values.
    [[nodiscard]] auto main() const -> el::conf::ValuePtr { return _document->valueOrThrow("main"_el); }
    /// Load the standard TLS section, including its own validation rules.
    [[nodiscard]] auto tlsProfiles() const -> el::TlsConfigurationSet {
        return el::TlsConfigurationParser{}.parse(_document->value("tls"_el));
    }
    /// Read an explicit command-line value or the validated configuration text.
    /// @param name The common field/option identifier.
    [[nodiscard]] auto text(const el::String &name) const -> el::String {
        if (_values->value(name)) {
            return _values->getText(name);
        }
        return main()->getTextOrThrow(name);
    }
    /// Read and validate a TLS profile label from configuration or the command line.
    /// @return A nonempty, bounded hierarchical label accepted by the TLS registry.
    [[nodiscard]] auto profile() const -> el::String {
        const auto label = text("tls_profile"_el);
        static const auto cLabel = el::re::RegEx::compile("[a-z0-9]+(-[a-z0-9]+)*(/[a-z0-9]+(-[a-z0-9]+)*){0,15}"_el);
        if (label.length() > el::ByteCount{255} || !cLabel->fullMatch(label)) {
            throw el::ApplicationError{"Invalid TLS profile label."_el};
        }
        return label;
    }
    /// Read a port override or the configuration's integer port as decimal text.
    /// @param name The common field/option identifier.
    [[nodiscard]] auto portText(const el::String &name) const -> el::String {
        if (_values->value(name)) {
            return _values->getText(name);
        }
        return el::String::fromInteger(main()->getIntegerOrThrow(name));
    }
    /// Resolve a configured file beside the chosen configuration.
    /// @param text Absolute or relative path text.
    [[nodiscard]] auto resolvePath(const el::String &text) const -> el::Path {
        auto path = el::Path{text};
        if (path.isAbsolute()) {
            return path;
        }
        return _path.parent() / path;
    }

private:
    el::OptionValuesPtr _values;  ///< Parsed command-line overrides.
    el::Path _path;               ///< Absolute configuration path.
    el::conf::ValuePtr _document; ///< Parsed configuration document.
};

}
