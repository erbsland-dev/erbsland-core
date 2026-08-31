// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Install manager limits as part of a complete logging configuration.
///
/// Construct `LogManagerOptions` as a value, customize it, and move it into `LogConfiguration`. Passing the completed
/// configuration to `setConfiguration()` makes these limits active together with the writer routes.
void managerOptions() {
    auto options = el::LogManagerOptions{};
    options.setMaximumEntries(2048U);

    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(std::move(options)).addWriter(std::make_shared<el::LastErrorsLogWriter>());

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto activeConfiguration = manager->configuration();
    el::io::printLine("Active queue-entry limit: "_el, activeConfiguration.managerOptions().maximumEntries());
    manager->shutdown();
}

}
