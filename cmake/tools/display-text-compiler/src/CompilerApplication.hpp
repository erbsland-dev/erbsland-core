// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/Literals.hpp>

namespace erbsland::i18n::compiler {

using namespace text::literals;

/// Command-line front end for the display-text compiler.
/// @notest{Executable entry point exercised by CMake integration fixtures.}
class CompilerApplication final : public core::Application {
    constexpr static auto cInputOption = "input"_el;
    constexpr static auto cHeaderOutputOption = "header-output"_el;
    constexpr static auto cSourceOutputOption = "source-output"_el;
    constexpr static auto cClassOption = "class"_el;
    constexpr static auto cNamespaceOption = "namespace"_el;

public:
    using Application::Application;

protected: // implement Application
    void registerCommandLineOptions(const options::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> unit::ExitCode override;
};

}
