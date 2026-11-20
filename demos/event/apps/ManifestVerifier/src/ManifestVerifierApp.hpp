// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace demo {

using namespace el::text::literals;

/// Verify the SHA-256 digests in a manifest.
class ManifestVerifierApp final : public el::Application {
    static constexpr auto cManifest = "manifest"_el;
    static constexpr auto cFailFast = "fail-fast"_el;

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    auto main() -> el::ExitCode override;

private:
    el::Path _manifest; ///< Absolute manifest path.
    bool _failFast{};   ///< Stop after a checksum mismatch.
};

}
