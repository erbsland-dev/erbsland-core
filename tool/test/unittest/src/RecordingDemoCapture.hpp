// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringJoins.hpp>
#include <tool/doc/impl/DemoCapture.hpp>

#include <utility>

/// Return the styled first argument in native and display path formats without launching a process.
class RecordingDemoCapture final : public tool::doc::impl::DemoCapture {
public:
    /// Supply deterministic raw terminal output for a capture test.
    /// @param text Raw UTF-8 and control sequences.
    explicit RecordingDemoCapture(el::String text) : _text{std::move(text)} {}

    // defaults
    RecordingDemoCapture() = default;

public: // implement DemoCapture
    auto run(const el::Path &, const el::StringList &arguments, const el::Path &, int, int) -> el::String override {
        if (!_text.isEmpty()) {
            return _text;
        }
        using namespace el::text::literals;
        return el::join(
            "\x1b[32m"_el,
            arguments.first(),
            "\n"_el,
            el::Path::fromNative(arguments.first()).toString(),
            "\x1b[0m\n"_el);
    }

private:
    el::String _text; ///< Optional raw capture fixture.
};
