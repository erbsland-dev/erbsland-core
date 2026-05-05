// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionRenderer.hpp"

namespace erbsland::options {

void OptionRenderer::displayHelp(const OptionsPtr &, text::StringView) {
    // do nothing by default
}

void OptionRenderer::displayVersion(const OptionsPtr &, text::StringView) {
    // do nothing by default
}

void OptionRenderer::displayError(const OptionsPtr &, const OptionErrorContext &) {
    // do nothing by default
}

}
