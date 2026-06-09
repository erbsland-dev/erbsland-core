// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionRendererBase.hpp"

namespace erbsland::options {

OptionRendererBase::OptionRendererBase() : _displayText{OptionDisplayText::defaultText()} {
}

OptionRendererBase::OptionRendererBase(OptionDisplayText displayText) : _displayText{std::move(displayText)} {
}

}
