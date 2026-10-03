// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParserStorage.hpp"

#include "OptionDisplayModel.hpp"

#include "../../i18n/DisplayText.hpp"

#include <utility>

namespace erbsland::options::impl {

using text::String;
using unit::ArgumentIndex;

OptionParserStorage::OptionParserStorage() {
}

auto OptionParserStorage::optionTitleForError(const OptionPtr &option) const -> String {
    return OptionDisplayModel::optionTitle(option);
}

auto OptionParserStorage::makeError(const OptionErrorReason reason, String description, const ArgumentIndex index)
    -> bool {
    _error = OptionErrorContext{}.setReason(reason).setTitle(std::move(description)).setArgumentIndex(index);
    return false;
}

auto OptionParserStorage::makeError(
    const OptionErrorReason reason,
    String title,
    String description,
    const ArgumentIndex index,
    const OptionPtr &option) -> bool {
    _error = OptionErrorContext{}
                 .setReason(reason)
                 .setTitle(std::move(title))
                 .setDescription(std::move(description))
                 .setArgumentIndex(index)
                 .setOption(option);
    return false;
}

auto OptionParserStorage::makeError(
    const OptionErrorReason reason, String description, const ArgumentIndex index, const OptionPtr &option) -> bool {
    _error = OptionErrorContext{}
                 .setReason(reason)
                 .setTitle(std::move(description))
                 .setArgumentIndex(index)
                 .setOption(option);
    return false;
}

}
