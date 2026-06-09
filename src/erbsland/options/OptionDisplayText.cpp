// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDisplayText.hpp"

#include "../text/Literals.hpp"

namespace erbsland::options {

using namespace text::literals;

OptionDisplayText::OptionDisplayText() :
    _applicationNameFallback{"application"_el},
    _usageLabel{"Usage: "_el},
    _modulePlaceholder{"<module>"_el},
    _optionsPlaceholder{"[options]"_el},
    _modulesHeading{"Modules"_el},
    _optionsHeading{"Options"_el},
    _helpOptionDescription{"Display this help."_el},
    _versionOptionDescription{"Display version information."_el},
    _integerPlaceholder{"<integer>"_el},
    _valuePlaceholder{"<value>"_el},
    _choicePlaceholder{"<choice>"_el},
    _placeholderPrefix{"<"_el},
    _placeholderSuffix{">"_el},
    _choicesLabel{"Choices: "_el},
    _versionLabel{"Version "_el},
    _authorLabel{"Author: "_el},
    _licenseLabel{"License: "_el},
    _errorLabel{"Error: "_el},
    _genericErrorMessage{"Option processing failed"_el},
    _moduleLabel{"Module: "_el},
    _optionLabel{"Option: "_el},
    _argumentIndexLabel{"Argument index: "_el} {
}

auto OptionDisplayText::defaultText() noexcept -> const OptionDisplayText & {
    static const auto result = OptionDisplayText{};
    return result;
}

}
