// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

#include <exception>

namespace demo {

void captureMusicalVariation(const el::TextOutputStreamPtr &outer, const el::TextOutputStreamPtr &inner);

void useNestedRedirects() {
    const auto outer = el::AnyStringBuilderStream::create();
    const auto inner = el::AnyStringBuilderStream::create();
    captureMusicalVariation(outer, inner);
    el::io::print("Outer:\n"_el, outer->takeString());
    el::io::print("Inner:\n"_el, inner->takeString());
    if (el::stdOut()->flush().isTimeout()) {
        throw el::RuntimeError{"Sending the captured sections timed out."_el};
    }
}

/// Compose standard-output redirections in strictly nested scopes.
/// Each guard restores the target that was active when it was created.
/// Destroy the guards in reverse order so the outer destination resumes after the inner capture finishes.
void captureMusicalVariation(const el::TextOutputStreamPtr &outer, const el::TextOutputStreamPtr &inner) {
    try {
        auto outerRedirect = el::redirectStdOut(outer);
        if (el::io::writeLine("Tema principale"_el).isTimeout()) {
            throw el::RuntimeError{"Capturing the theme timed out."_el};
        }
        {
            auto innerRedirect = el::redirectStdOut(inner);
            if (el::io::writeLine("Variazione veloce"_el).isTimeout()) {
                throw el::RuntimeError{"Capturing the variation timed out."_el};
            }
        }
        if (el::io::writeLine("Ripresa del tema"_el).isTimeout()) {
            throw el::RuntimeError{"Capturing the reprise timed out."_el};
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The musical sections could not be captured."_el, std::current_exception()};
    }
}

}
