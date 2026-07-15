// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ResultDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ClearStatuses"_el, clearStatuses);
    app.registerDemo("CustomResultTypes"_el, customResultTypes);
    app.registerDemo("HandlingResults"_el, handlingResults);
    app.registerDemo("ResultOrException"_el, resultOrException);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
