// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfileTypes.hpp"

#include "impl/ConfigurationLoader.hpp"

namespace app::regex {

auto Configuration::load(const std::optional<el::Path> &path) -> Configuration {
    return impl::ConfigurationLoader::load(path);
}

void Configuration::writeTemplate(const el::Path &path) {
    impl::ConfigurationLoader::writeTemplate(path);
}

}
