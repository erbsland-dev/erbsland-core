// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

#include <exception>
#include <vector>

namespace demo {

/// Finish accepted satellite observations before ending dispatch; reject later work.
void drainAcceptedWork() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto observations = el::StringList{};
    loop->invoke([&observations]() -> void { observations.append("Molntäcke"_el); });
    loop->invoke([&observations]() -> void { observations.append("Havsis"_el); });
    loop->quit();
    loop->invoke([&observations]() -> void { observations.append("Snödjup"_el); });
    // Run the accepted callbacks.
    loop->run();

    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (observations != el::StringList{"Molntäcke"_el, "Havsis"_el}) {
        throw el::LogicFailure{"Quit did not preserve exactly the accepted observations."_el};
    }
    for (const auto &observation : observations) {
        el::io::printLine("Recorded: "_el, observation);
    }
}

}
