// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ExceptionDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ApplicationReporting"_el, applicationReporting);
    app.registerDemo("BuiltInExceptions"_el, builtInExceptions);
    app.registerDemo("CauseChains"_el, causeChains);
    app.registerDemo("ChoosingFailureMechanism"_el, choosingFailureMechanism);
    app.registerDemo("ContextPattern"_el, contextPattern);
    app.registerDemo("DomainExceptions"_el, domainExceptions);
    app.registerDemo("ExceptionText"_el, exceptionText);
    app.registerDemo("MinimalCustomException"_el, minimalCustomException);
    app.registerDemo("ThrowAndCatch"_el, throwAndCatch);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
