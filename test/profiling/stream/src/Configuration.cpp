// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Configuration.hpp"

#include "DefaultConfiguration.hpp"

#include "impl/ConfigurationTools.hpp"

#include <erbsland/conf/Parser.hpp>

namespace app::stream {

auto ConfigurationLoader::load(const std::optional<el::Path> &path) -> Configuration {
    auto parser = el::conf::Parser{};
    const auto defaultText = el::String{cDefaultConfigurationText};
    const auto defaultDocument = parser.parseTextOrThrow(defaultText);
    const auto defaults = impl::parseConfigurationDocument(defaultDocument);
    if (!path) {
        return defaults;
    }
    const auto document = parser.parseFileOrThrow(*path);
    return impl::parseConfigurationDocument(document, &defaults);
}

void ConfigurationLoader::writeTemplate(const el::Path &path) {
    auto options = el::PathWriteTextOptions{el::StringEncoding::Utf8};
    options.setCreateParents(true).setBomMode(el::StringBomMode::Reject);
    path.content().writeTextOrThrow(el::String{cDefaultConfigurationText}, options);
}

}
